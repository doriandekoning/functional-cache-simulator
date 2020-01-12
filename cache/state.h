#ifndef STATE_H
#define STATE_H
#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "pipereader/pipereader.h"
#include "coherency_protocol.h"

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



struct CacheState {
	struct CacheState* higher_level_cache;
	size_t amount_lower_level_caches; //TODO rename lower level
	size_t cur_size_lower_level_caches_array;
	struct CacheState** lower_level_caches; // List of lower_level_caches
	bool write_back;
	int associativity;
	size_t size; // Size in lines //TODO during setup verify this is a multiple of lower level caches
	size_t line_size; // Cache line size in bytes
	//TODO implement different caches for data and instructions
	struct CacheLine* lines;
	struct Bus* bus;
	int (*eviction_func)(struct CacheState*, uint64_t);
	struct CoherencyProtocol* coherency_protocol;
};

typedef int (*EvictionFunc)(struct CacheState*, uint64_t);



// Sets up a new cache state with the following parameters
// - higer level cache: is the higher level cache of the new cache, the new cache will be added to the list of lower level caches in the higher level cache (if NULL is provided the cache is LLC)
// - write_back: true if cache is write back, false if cache is write througn
// - size: the size of the cache in the amount of lines, the size of the higher_level_cache should be a multiple of this
// - line_size: the cache line size in bytes
struct CacheState* setup_cachestate(struct CacheState* higher_level_cache, bool write_back, size_t size, size_t line_size, int associativity, EvictionFunc evictionfunc, struct CoherencyProtocol* coherency_protocol);

void free_cachestate(struct CacheState* state);

void add_lower_level_cache(struct CacheState* higher_level_cache, struct CacheState* lower_level_cache);

void access_cache(struct CacheState* state, uint64_t address, uint64_t timestamp, bool write);

int get_line_location_in_cache(struct CacheState* state, uint64_t address);


#endif /* STATE_H */
