#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "state.h"
#include "bus.h"

struct CacheState* setup_cachestate(struct CacheState* parent, bool write_back, size_t size, size_t line_size, int associativity, EvictionFunc evictionfunc, NewStateFunc new_state_func) {

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
	new_state->eviction_func = evictionfunc;
	new_state->new_state_func = new_state_func;
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

void register_cache_miss(bool write, uint64_t address) {
	printf("Cache miss to address:%lu\n", address);
}

int get_line_location_in_cache(struct CacheState* state, uint64_t address) {
	int set_idx = CALCULATE_SET_INDEX(state, address);
	for(int i = set_idx; i < (set_idx + state->associativity); i++) {
		if(CALCULATE_TAG(state, address) == state->lines[i].tag && state->lines[i].state != CACHELINE_STATE_INVALID) {
			return i;
		}
	}
	return -1;
}

void access_cache(struct CacheState* state, uint64_t address, uint64_t timestamp, bool write) {
	int lru_idx;
	int line_idx = get_line_location_in_cache(state, address);
	int old_state = CACHELINE_STATE_INVALID;
	if(line_idx >= 0) {
		// Cache hit, update lru
		state->lines[line_idx].last_used = timestamp;
		old_state = state->lines[line_idx].state;
	}else{
		if(state->parent_cache != NULL) {
			// Cache miss, search parent cache
			access_cache(state->parent_cache, address, timestamp, write);
		}else{
			// Cache miss but there is no parent cache, register miss
			register_cache_miss(true, address);
		}
		// Fill cache, update the new entry (and evict the oldest one)
		line_idx = state->eviction_func(state, address);
		state->lines[line_idx].tag = CALCULATE_TAG(state, address);
		state->lines[line_idx].last_used = timestamp;
	}
	int event;
	if(write) {
		event = CACHE_EVENT_WRITE;
	}else{
		event = CACHE_EVENT_READ;
	}
	// Inform bus of eviction (parents will also be snooping the bus and thus be notified)
	int bus_req;
	state->lines[line_idx].state = state->new_state_func(old_state, event, &bus_req);
	if(state->bus != NULL && bus_req>=0) {
		handle_bus_event(state->bus, bus_req, address);
		//TODO handle flush
	}
}
