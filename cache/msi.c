#include "msi.h"
#include "state.h"

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




// struct statechange get_msi_state_change_by_bus_request(int current_state, int bus_request){
// 	struct statechange ret;
// 	ret.bus_request = BUS_REQUEST_NOTHING;
// 	if(current_state == STATE_INVALID) {
// 		ret.new_state = STATE_INVALID;
// 	} else if(current_state == STATE_SHARED) {
// 		if(bus_request == BUS_REQUEST_READ) {
// 			ret.new_state = STATE_SHARED;
// 		} else {
// 			ret.new_state = STATE_INVALID;
// 		}
// 	} else if(current_state == STATE_MODIFIED) {
// 		ret.bus_request = BUS_REQUEST_FLUSH;
// 		if(bus_request == BUS_REQUEST_READ) {
// 			ret.new_state = STATE_SHARED;
// 		} else {
// 			ret.new_state = STATE_INVALID;
// 		}
// 	} else{
// 		printf("State transition unknown!\n");
// 		exit(7);
// 	}
// 	return ret;
// }
