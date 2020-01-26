#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "bus.h"
#include "state.h"
#include "coherency_protocol.h"

struct Bus* init_bus() {
    struct Bus* bus = malloc(sizeof(struct Bus));
    bus->amount_caches = 0;
    bus->cur_size_caches_array = 8;
    bus->caches = malloc(8*sizeof(struct CacheState*));
    return bus;
}

void free_bus(struct Bus* bus){
    free(bus->caches);
    free(bus);
}

void add_cache_to_bus(struct Bus* bus, struct CacheState* cache) {
	// Check if array needs to be sized up
	if(bus->amount_caches == bus->cur_size_caches_array){
        printf("TEst!\n");

		struct CacheState** old_caches = bus->caches;
		bus->caches = malloc(sizeof(struct CacheState*)* (bus->cur_size_caches_array+8));
		for(int i = 0; i < bus->cur_size_caches_array; i++) {
			bus->caches[i] = old_caches[i];
		}
		bus->cur_size_caches_array += 8;
		free(old_caches);
	}
	bus->caches[bus->amount_caches] = cache;
	bus->amount_caches++;
    cache->bus = bus;
}

void handle_bus_event(struct Bus* bus, int event, uint64_t address, struct CacheState* origin_cache){
    struct CacheState* cur_cache;
    int bus_request;
    for(int i = 0; i < bus->amount_caches; i++){
        cur_cache = bus->caches[i];
        if(cur_cache == origin_cache) continue;
        int cache_line_idx = get_line_location_in_cache(cur_cache, address);

        int old_state = CACHELINE_STATE_INVALID;
        if(cache_line_idx == -1){
            cache_line_idx = cur_cache->eviction_func(cur_cache, address);
        }else{
            old_state = cur_cache->lines[cache_line_idx].state;
        }


        int new_state = cur_cache->coherency_protocol->new_state_func(old_state, event, &bus_request);
        cur_cache->lines[cache_line_idx].state = new_state;//TODO check if this updates LRU
        if(bus_request != CACHE_EVENT_NONE){
            //TODO handle
            // printf("Unhandled bus request!\n");
        }

    }
}
