#include <stdio.h>

#include "msi.h"
#include "state.h"



struct CoherencyProtocol msi_coherency_protocol = {
    .new_state_func = &new_state_msi,
    .flush_needed_on_evict = &flush_needed_on_eviction_msi,
};

int new_state_msi(int old_state, int event, int* bus_request){
	*bus_request = -1;
	if(event == CACHE_EVENT_WRITE) {
		if(old_state == CACHELINE_STATE_INVALID) {
			*bus_request = CACHE_EVENT_BUS_READ_X;
			return CACHELINE_STATE_MODIFIED;
		} else if(old_state == CACHELINE_STATE_SHARED) {
			*bus_request = CACHE_EVENT_BUS_UPGRADE;
			return CACHELINE_STATE_MODIFIED;
		} else if(old_state == CACHELINE_STATE_MODIFIED) {
			return CACHELINE_STATE_MODIFIED;
		}
	} else if(event == CACHE_EVENT_READ) {
		if(old_state == CACHELINE_STATE_INVALID){
			*bus_request = CACHE_EVENT_BUS_READ;
			return CACHELINE_STATE_SHARED;
		} else if(old_state == CACHELINE_STATE_SHARED){
			return CACHELINE_STATE_SHARED;
		} else if(old_state == CACHELINE_STATE_MODIFIED){
			return CACHELINE_STATE_MODIFIED;
		}
	} else if(event == CACHE_EVENT_BUS_READ) {
		if(old_state == CACHELINE_STATE_INVALID){
			return CACHELINE_STATE_INVALID;
		} else if(old_state == CACHELINE_STATE_SHARED){
			return CACHELINE_STATE_SHARED;
		} else if(old_state == CACHELINE_STATE_MODIFIED){
			return CACHELINE_STATE_SHARED;
		}
	} else if(event == CACHE_EVENT_BUS_READ_X || event == CACHE_EVENT_BUS_UPGRADE) {
		return CACHELINE_STATE_INVALID;
	} else if(CACHE_EVENT_BUS_FLUSH) {
		//TODO not sure what to do here
	} else {
        printf("Event not supported!\n");
    }
    return CACHE_EVENT_NONE;
}

bool flush_needed_on_eviction_msi(int cur_state) {
	return cur_state == CACHELINE_STATE_MODIFIED;
}
