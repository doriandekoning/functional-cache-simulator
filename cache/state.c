#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "state.h"

uint64_t ADDRESS_OFFSET_MASK = 0;
uint64_t ADDRESS_TAG_MASK = 0;
uint64_t ADDRESS_INDEX_MASK = 0xfffff;
uint64_t setupmask(unsigned from, unsigned to) {
        uint64_t mask = 0;
        for(unsigned i = from; i <=to; i++) {
                mask |= (1ULL << i);
        }
        return mask;
}

const int STATE_INVALID = 0;
const int STATE_SHARED = 1;
const int STATE_MODIFIED = 2;

const int CACHELINE_STATE_INVALID = 0;
const int CACHELINE_STATE_SHARED = 1;
const int CACHELINE_STATE_MODIFIED = 2;

const int BUS_REQUEST_NOTHING = 0;
const int BUS_REQUEST_READ = 1;
const int BUS_REQUEST_READX = 2;
const int BUS_REQUEST_UPGR = 3;
const int BUS_REQUEST_FLUSH = 4;


int get_line_state(CacheState state, uint64_t cpu, uint64_t address, uint64_t* line_index) {

	int set_index = (cpu * CACHE_AMOUNT_LINES * ASSOCIATIVITY) + ((address & ADDRESS_INDEX_MASK) * ASSOCIATIVITY);
	for(int i = 0; i < ASSOCIATIVITY; i++) {
		if(state[set_index+i].tag == (address & ADDRESS_TAG_MASK)){
			if(line_index != NULL) {
				*line_index = set_index+i;
			}
			return state[set_index+i].state;
		}
	}
	return STATE_INVALID;
}


void handle_bus_request(CacheState state, uint64_t address, int origin_cpu, int request) {
	if(request == BUS_REQUEST_NOTHING){
		return;
	}

	int next_bus_req = BUS_REQUEST_NOTHING;
	int next_bus_req_cpu;

	for(int cpu_idx = 0; cpu_idx < AMOUNT_SIMULATED_PROCESSORS; cpu_idx++){
		if(cpu_idx == origin_cpu)continue;
		uint64_t line_index;
		int cur_state = get_line_state(state, cpu_idx, address, &line_index);

		struct statechange state_change = get_msi_state_change_by_bus_request(cur_state, request);

		if(state_change.new_state != cur_state) {
			state[line_index].state = state_change.new_state;
		}
		if(state_change.bus_request != BUS_REQUEST_NOTHING){
			next_bus_req = state_change.bus_request;
			next_bus_req_cpu = cpu_idx;
		}
	}

	if(next_bus_req != BUS_REQUEST_NOTHING) {
		handle_bus_request(state, address, next_bus_req_cpu, next_bus_req);
	}
}

bool perform_cache_access(CacheState state, uint64_t cpu, uint64_t address, bool write) {
	int current_state = STATE_INVALID;
	int index = (cpu * CACHE_AMOUNT_LINES * ASSOCIATIVITY) + ((address & ADDRESS_INDEX_MASK) * ASSOCIATIVITY);
	bool found = false;
	for(int i = 0; i < ASSOCIATIVITY; i++){
		struct CacheLine* entry = &(state[index + i]);
		if(entry->state != STATE_INVALID){
			if(entry->tag == (address & ADDRESS_TAG_MASK)) {
				//Found
				current_state = entry->state;
				struct statechange state_change = get_msi_state_change(current_state, write);
				entry->state = state_change.new_state;
				for(int j = 0; j < ASSOCIATIVITY; j++) {
					if(j==i) continue;
					if(state[index+j].state != STATE_INVALID && state[index+j].lru < entry->lru)state[index+j].lru++;
				}
				entry->lru = 0;
				handle_bus_request(state, address, cpu, state_change.bus_request);
				return true;
			}
		}
	}

	struct statechange state_change = get_msi_state_change(STATE_INVALID, write);
	int lru_index = -1;
	int highest_lru_value = -1;
	// See if there is an INVALID entry
	for(int i = 0; i < ASSOCIATIVITY; i++) {
		if(state[index+i].state == STATE_INVALID){
			state[index+i].tag = (address & ADDRESS_TAG_MASK);
			state[index+i].state = state_change.new_state;
			state[index+i].lru = 0;
			for(int j = 0; j < ASSOCIATIVITY; j++) {
				if(i == j) continue;
				if(state[index+j].state != STATE_INVALID)state[index+j].lru++;
			}
			handle_bus_request(state, address, cpu, state_change.bus_request);
			return false;
		}
		if(state[index+i].lru > highest_lru_value) {
			highest_lru_value = state[index+i].lru;
			lru_index = i;
		}
	}
	// No invalid entry found overwrite the entry with the higest lru index
	state[index+lru_index].state = state_change.new_state;
	state[index+lru_index].lru = 0;
	state[index+lru_index].tag = (address & ADDRESS_TAG_MASK);
	for(int i = 0; i < ASSOCIATIVITY; i++) {
		if(i == lru_index) continue;
		state[index+i].lru++;
	}
	handle_bus_request(state, address, cpu, state_change.bus_request);
	return false;
}



void init_cachestate_masks(int indexsize, int offsetsize) {
        ADDRESS_OFFSET_MASK = setupmask(0,(offsetsize-1));
        ADDRESS_TAG_MASK = setupmask(offsetsize+indexsize,63);
        ADDRESS_INDEX_MASK = setupmask(offsetsize,(indexsize+offsetsize-1));
}


struct statechange get_msi_state_change(int current_state, bool write) {
	struct statechange ret;
	if(write) {
		if(current_state == STATE_INVALID) {
			//PrWrI: accessing processor to M and issues BusRdX
			ret.new_state = STATE_MODIFIED;
			ret.bus_request = BUS_REQUEST_READX;
		} else if(current_state == STATE_SHARED) {
			//PrWrS: that processor to M others to I
			ret.new_state = STATE_MODIFIED;
			ret.bus_request = BUS_REQUEST_UPGR;
		} else if(current_state == STATE_MODIFIED) {
			//PrWrM: accessing processor to M others stays same
			ret.new_state = STATE_MODIFIED;
			ret.bus_request = BUS_REQUEST_NOTHING;
		} else {
			ret.new_state = -1;
			ret.bus_request = -1;
		}
	} else {
		if(current_state == STATE_INVALID){
			//PrRdI: accessing processor to S others to S
			ret.new_state = STATE_SHARED;
			ret.bus_request = BUS_REQUEST_READ;
		} else if(current_state == STATE_SHARED){
			ret.new_state = STATE_SHARED;
			ret.bus_request = BUS_REQUEST_NOTHING;
		} else if(current_state == STATE_MODIFIED){
			ret.new_state = STATE_MODIFIED;
			ret.bus_request = BUS_REQUEST_NOTHING;
		} else {
			ret.new_state = -1;
			ret.bus_request = -1;
		}
	}
	return ret;
}

struct statechange get_msi_state_change_by_bus_request(int current_state, int bus_request){
	struct statechange ret;
	ret.bus_request = BUS_REQUEST_NOTHING;
	if(current_state == STATE_INVALID) {
		ret.new_state = STATE_INVALID;
	} else if(current_state == STATE_SHARED) {
		if(bus_request == BUS_REQUEST_READ) {
			ret.new_state = STATE_SHARED;
		} else {
			ret.new_state = STATE_INVALID;
		}
	} else if(current_state == STATE_MODIFIED) {
		ret.bus_request = BUS_REQUEST_FLUSH;
		if(bus_request == BUS_REQUEST_READ) {
			ret.new_state = STATE_SHARED;
		} else {
			ret.new_state = STATE_INVALID;
		}
	} else{
		printf("State transition unknown!\n");
		exit(7);
	}
	return ret;
}




struct CacheState* setup_cachestate(struct CacheState* parent, bool write_back, size_t size, size_t line_size, uint associativity) {

	struct CacheState* new_state = malloc(sizeof(struct CacheState));
	if(parent != NULL) {
		if(parent->size % size != 0){
			printf("Error during cache setup: Parent size %d is not a multiple of size: %d\n", parent->size, size);
			return NULL;
		}
		if(parent->line_size != line_size) {
			printf("Error during cache setup: Parent line size should be equal (%d) but is not (%d)\n", parent->line_size, line_size);
			return NULL;
		}
		add_child(parent, new_state);
	}

	new_state->amount_children = 0;
	new_state->children = NULL;
	new_state->write_back = write_back;
	new_state->size = size;
	new_state->line_size = line_size;
	new_state->cur_size_children_array = 0;
	new_state->lines = malloc(sizeof(struct CacheLine) * size);
	if(associativity > size || associativity == 0) {
		printf("Error during cache setup: invalid value for associativity: %d, value should be > 0 and < size\n", associativity);
		return NULL;
	}
	new_state->associativity = associativity;
	for(int i = 0; i < size; i++) {
		new_state->lines[i].state = CACHELINE_STATE_INVALID;
	}
	return new_state;

}

void add_child(struct CacheState* parent, struct CacheState* child) {
	// Check if array needs to be sized up
	if(parent->amount_children == parent->cur_size_children_array){
		struct CacheState** old_children = parent->children;
		parent->children = malloc(sizeof(struct CacheState*)* (parent->cur_size_children_array+8));
		for(int i = 0; i < parent->cur_size_children_array; i++) {
			parent->children[i] = old_children[i];
		}
		parent->cur_size_children_array += 8;
		free(old_children);
	}
	parent->children[parent->amount_children] = child;
	parent->amount_children++;
	child->parent_cache = parent;
}

void free_cachestate(struct CacheState* state) {
	free(state->lines);
	free(state->children);
	free(state);
}

int get(struct CacheState* state, uint64_t address) {
	int set_idx = CALCULATE_SET_INDEX(state, address);
	for(int i = set_idx; i < (set_idx + state->associativity); i++) {
		// if(state->lines[i]->tag )
	}
	return 0;
}

void write(struct CacheState* state, uint64_t address) {
	// int line_idx = read();
}
