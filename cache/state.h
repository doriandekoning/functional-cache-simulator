#ifndef STATE_H
#define STATE_H
#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "pipereader/pipereader.h"

#define CACHELINE_STATE_INVALID 0

#define CACHE_EVENT_NONE 0
#define CACHE_EVENT_READ 1
#define CACHE_EVENT_WRITE 2


/*#define CACHE_WRITE (uint8_t)1
#define CACHE_READ  (uint8_t)2
#define CR_UPDATE (uint8_t)3*/


#define CALCULATE_SET_INDEX(state, address) (((address/state->line_size) * state->associativity) % state->size)
#define CALCULATE_TAG(state, address) (((address/state->line_size)  * state->associativity) / state->size)

struct CacheLine {
	uint64_t tag;
	uint8_t state;
	uint64_t last_used; //TODO refactor to more generic "eviction info"
}; //TODO check if packing increases performance

struct statechange {
	int new_state;
	int bus_request;
};


typedef struct CacheLine* CacheState;
typedef int (*NewStateFunc)(int old_state, int event, int* bus_request);

struct CacheState {
	struct CacheState* parent_cache;
	size_t amount_children;
	size_t cur_size_children_array;
	struct CacheState** children; // List of children
	bool write_back;
	int associativity;
	size_t size; // Size in lines //TODO during setup verify this is a multiple of child caches
	size_t line_size; // Cache line size in bytes
	//TODO implement different caches for data and instructions
	struct CacheLine* lines;
	struct Bus* bus;
	int (*eviction_func)(struct CacheState*, uint64_t);
	NewStateFunc new_state_func;
};

typedef int (*EvictionFunc)(struct CacheState*, uint64_t);



// Sets up a new cache state with the following parameters
// - parent: is the higher level cache of the new cache, the new cache will be added to the list of children in the parent (if NULL is provided the cache is LLC)
// - write_back: true if cache is write back, false if cache is write througn
// - size: the size of the cache in the amount of lines, the size of the parent should be a multiple of this
// - line_size: the cache line size in bytes
struct CacheState* setup_cachestate(struct CacheState* parent, bool write_back, size_t size, size_t line_size, int associativity, EvictionFunc evictionfunc, NewStateFunc new_state_func);

void free_cachestate(struct CacheState* state);

void add_child(struct CacheState* parent, struct CacheState* child);

void access_cache(struct CacheState* state, uint64_t address, uint64_t timestamp, bool write);

int get_line_location_in_cache(struct CacheState* state, uint64_t address);


#endif /* STATE_H */