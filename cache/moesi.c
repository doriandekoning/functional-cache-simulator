// #include <stdio.h>

// #include "moesi.h"
// #include "state.h"



// struct CoherencyProtocol moesi_coherency_protocol = {
//     .new_state_func = &new_state_moesi,
//     .flush_needed_on_evict = &flush_needed_on_eviction_moesi,
// };

// int new_state_moesi(int old_state, int event, int* bus_request){
// 	*bus_request = -1;
// 	if(old_state == CACHELINE_STATE_INVALID) {
// 		if(event == CACHE_EVENT_READ) {
// 			*bus_request = MOESI_CACHE_EVENT_BUS_READ;
// 			// Look in other caches, if one has this entry, it gets shared
// 			// If none have it it becomes exclusive
// 		}else if(event == CACHE_EVENT_WRITE) {
// 			*bus_request = MOESI_CACHE_EVENT_BUS_READ_X;
// 			return MOESI_STATE_MODIFIED;
// 		}
// 		// Bus signals ignored
// 	}else if(old_state == MOESI_STATE_EXCLUSIVE) {
// 		if(event == CACHE_EVENT_WRITE) {
// 			//no bus request
// 			return MOESI_STATE_MODIFIED;
// 		}else if(event == CACHE_EVENT_READ) {
// 			*bus_request = MOESI_CACHE_EVENT_BUS_FLUSH;
// 			return MOESI_STATE_SHARED;
// 		}else if(event == MOESI_CACHE_EVENT_BUS_READ_X) {
// 			*bus_request = MOESI_CACHE_EVENT_BUS_FLUSH;
// 			return CACHELINE_STATE_INVALID;
// 		}
// 	}else if(old_state == MOESI_STATE_SHARED){
// 		if(event == CACHE_EVENT_WRITE) {
// 			*bus_request = MOESI_CACHE_EVENT_BUS_UPGRADE;
// 			return MOESI_STATE_MODIFIED;
// 		}else if(event == MOESI_CACHE_EVENT_BUS_READ) {
// 			*bus_request = MOESI_CACHE_EVENT_BUS_FLUSH;
// 			return MOESI_STATE_SHARED;
// 		}else if(event == MOESI_CACHE_EVENT_BUS_READ_X) {
// 			*bus_request = MOESI_CACHE_EVENT_BUS_FLUSH;
// 			return CACHELINE_STATE_INVALID;
// 		}
// 		//Todo bus requests
// 	}else if (old_state == MOESI_STATE_MODIFIED) {
// 		//No state transititon
// 		if(event == MOESI_CACHE_EVENT_BUS_READ) {
// 			*bus_request = MOESI_CACHE_EVENT_BUS_FLUSH;
// 			return MOESI_STATE_OWNED;
// 		}else if(event == MOESI_CACHE_EVENT_BUS_READ_X) {
// 			*bus_request = MOESI_CACHE_EVENT_BUS_FLUSH;
// 			return CACHELINE_STATE_INVALID;
// 		}
// 	}else if (old_state == MOESI_STATE_OWNED) {
// 		if(event == CACHE_EVENT_WRITE) {
// 			*bus_request = MOESI_CACHE_EVENT_BUS_UPGRADE;
// 			return MOESI_STATE_MODIFIED;
// 		}else if(event == MOESI_CACHE_EVENT_BUS_READ) {
// 			*bus_request = MOESI_CACHE_EVENT_BUS_FLUSH;
// 			return MOESI_STATE_OWNED;
// 		}else if(event == MOESI_CACHE_EVENT_BUS_UPGRADE) {
// 			*bus_request = MOESI_CACHE_EVENT_BUS_FLUSH;
// 			return CACHELINE_STATE_INVALID;
// 		}
// 	}
//     return CACHE_EVENT_NONE;
// }

// bool flush_needed_on_eviction_moesi(int cur_state) {
// 	return false;
// 	// return cur_state == CACHELINE_STATE_EXCLUSIVE;
// }
