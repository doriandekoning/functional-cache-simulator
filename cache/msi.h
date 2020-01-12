#ifndef __MSI_H
#define __MSI_H

#include <stdbool.h>

//States
//Invalid is globally defined in state
#define CACHELINE_STATE_SHARED 1
#define CACHELINE_STATE_MODIFIED 2

// Events
#define CACHE_EVENT_BUS_READ 3 // Read request from other processor
#define CACHE_EVENT_BUS_READ_X 4 // Read request with intention to write from other processor
#define CACHE_EVENT_BUS_UPGRADE 5 // Write hit in cache, send out to invalidate other caches
#define CACHE_EVENT_BUS_FLUSH 6// Indicates cahce line is being written back

int new_state_msi(int old_state, int event, int* bus_request);
bool flush_needed_on_eviction_msi(int cur_state);

#endif /*__MSI_H*/
