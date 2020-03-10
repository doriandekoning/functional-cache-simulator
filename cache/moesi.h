// #ifndef __MOESI_H
// #define __MOESI_H

// #include <stdbool.h>

// //States
// //Invalid is globally defined in state
// #define MOESI_STATE_SHARED 1
// #define MOESI_STATE_MODIFIED 2
// #define MOESI_STATE_OWNED 3
// #define MOESI_STATE_EXCLUSIVE 4

// // Events
// #define MOESI_CACHE_EVENT_BUS_READ 3 // Read request from other processor
// #define MOESI_CACHE_EVENT_BUS_READ_X 4 // Read request with intention to write from other processor
// #define MOESI_CACHE_EVENT_BUS_UPGRADE 5 // Write hit in cache, send out to invalidate other caches
// #define MOESI_CACHE_EVENT_BUS_FLUSH 6// Indicates cahce line is being written back

// extern struct CoherencyProtocol moesi_coherency_protocol;

// int new_state_moesi(int old_state, int event, int* bus_request);
// bool flush_needed_on_eviction_moesi(int cur_state);

// #endif /*__MOESI_H*/
