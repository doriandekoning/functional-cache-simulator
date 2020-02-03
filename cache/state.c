#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "state.h"
#include "bus.h"

struct CacheState* setup_cachestate(struct CacheState* higher_level_cache, bool write_back, size_t size, size_t line_size, uint32_t associativity, EvictionFunc evictionfunc, struct CoherencyProtocol* coherency_protocol, CacheMissFunc cache_miss_func){

	struct CacheState* new_state = malloc(sizeof(struct CacheState));
	if(higher_level_cache != NULL) {
		if(higher_level_cache->size % size != 0){
			printf("Error during cache setup: higher_level_cache size %ld is not a multiple of size: %ld\n", higher_level_cache->size, size);
			return NULL;
		}
		if(higher_level_cache->line_size != line_size) {
			printf("Error during cache setup: higher_level_cache line size should be equal (%ld) but is not (%ld)\n", higher_level_cache->line_size, line_size);
			return NULL;
		}
		add_lower_level_cache(higher_level_cache, new_state);
	}

	new_state->amount_lower_level_caches = 0;
	new_state->lower_level_caches = NULL;
	new_state->write_back = write_back;
	new_state->size = size;
	new_state->line_size = line_size;
	new_state->cur_size_lower_level_caches_array = 0;
	new_state->lines = malloc(sizeof(struct CacheLine) * size);
	new_state->eviction_func = evictionfunc;
	new_state->cache_miss_func = cache_miss_func;
	new_state->coherency_protocol = coherency_protocol;
	if(associativity > size || associativity == 0) {
		printf("Error during cache setup: invalid value for associativity: %d, value should be > 0 and < size\n", associativity);
		return NULL;
	}
	new_state->associativity = associativity;
	for(size_t i = 0; i < size; i++) {
		new_state->lines[i].state = CACHELINE_STATE_INVALID;
		new_state->lines[i].tag = -1;
		new_state->lines[i].last_used = 0;
	}
	return new_state;
}

void add_lower_level_cache(struct CacheState* higher_level_cache, struct CacheState* lower_level_cache) {
	// Check if array needs to be sized up

	if(higher_level_cache->amount_lower_level_caches == higher_level_cache->cur_size_lower_level_caches_array){

		struct CacheState** old_lower_level_caches = higher_level_cache->lower_level_caches;
		higher_level_cache->lower_level_caches = malloc(sizeof(struct CacheState*)* (higher_level_cache->cur_size_lower_level_caches_array+8));
		for(size_t i = 0; i < higher_level_cache->cur_size_lower_level_caches_array; i++) {
			higher_level_cache->lower_level_caches[i] = old_lower_level_caches[i];
		}
		higher_level_cache->cur_size_lower_level_caches_array += 8;
		free(old_lower_level_caches);
	}

	higher_level_cache->lower_level_caches[higher_level_cache->amount_lower_level_caches] = lower_level_cache;

	higher_level_cache->amount_lower_level_caches++;
	lower_level_cache->higher_level_cache = higher_level_cache;
}

void free_cachestate(struct CacheState* state) {
	free(state->lines);
	free(state->lower_level_caches);
	free(state);
}

void register_cache_miss(bool write, uint64_t address) {
	(void)(write);
	(void)(address);
	debug_printf("Cache miss to address%d:%016lx\n", write, address);
}

int get_line_location_in_cache(struct CacheState* state, uint64_t address) {
	int set_idx = CALCULATE_SET_INDEX(state, address);
	for(int i = set_idx; i < (set_idx + state->associativity); i++) {
		if(CALCULATE_TAG(state, address) == state->lines[i].tag ) {
			return i;
		}
	}
	return -1;
}

void inform_lower_level_caches_eviction(struct CacheState* state, uint64_t address) {
	for(size_t i = 0; i < state->amount_lower_level_caches; i++) {
		int lower_level_cache_line_idx = get_line_location_in_cache(state->lower_level_caches[i], address);
		if(lower_level_cache_line_idx >= 0){
			state->lower_level_caches[i]->lines[lower_level_cache_line_idx].state = CACHELINE_STATE_INVALID;
		}
	}
}

void access_cache(struct CacheState* state, uint64_t address, uint64_t timestamp, bool write) {
	debug_printf("Accessing cache state address:%lx, write: %d\n", address, write);
	int line_idx = get_line_location_in_cache(state, address);
	int old_state = CACHELINE_STATE_INVALID;
	if(line_idx >= 0) {
		// Cache hit, update lru
		old_state = state->lines[line_idx].state;
	}else{
		if(state->higher_level_cache != NULL) {
			// Cache miss, search higher_level_cache cache
			access_cache(state->higher_level_cache, address, timestamp, write);
		}else{
			// Cache miss but there is no higher_level_cache cache, register miss
			state->cache_miss_func(write, timestamp, address);
		}
		// Fill cache, update the new entry (and evict the oldest one)
		line_idx = state->eviction_func(state, address);
		inform_lower_level_caches_eviction(state, address);
		if(state->coherency_protocol->flush_needed_on_evict(state->lines[line_idx].state)){
			state->cache_miss_func(write, timestamp, address);
		}
		state->lines[line_idx].tag = CALCULATE_TAG(state, address);
	}
	state->lines[line_idx].last_used = timestamp;
	int event;
	if(write) {
		event = CACHE_EVENT_WRITE;
	}else{
		event = CACHE_EVENT_READ;
	}

	// Inform other caches on bus of eviction
	int bus_req;
	int new_state = state->coherency_protocol->new_state_func(old_state, event, &bus_req);
	state->lines[line_idx].state = new_state;

	if(state->bus != NULL && bus_req>=0) {
		handle_bus_event(state->bus, bus_req, address, state);
	}

}


