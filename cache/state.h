#ifndef STATE_H
#define STATE_H
#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "pipereader/pipereader.h"

extern const int STATE_INVALID;
extern const int STATE_INVALID;
extern const int STATE_SHARED;
extern const int STATE_MODIFIED;

extern const int BUS_REQUEST_NOTHING;
extern const int BUS_REQUEST_READ;
extern const int BUS_REQUEST_READX;
extern const int BUS_REQUEST_UPGR;
extern const int BUS_REQUEST_FLUSH;

#define CACHE_WRITE (uint8_t)1
#define CACHE_READ  (uint8_t)2
#define CR_UPDATE (uint8_t)3


extern uint64_t ADDRESS_OFFSET_MASK;
extern uint64_t ADDRESS_TAG_MASK;
extern uint64_t ADDRESS_INDEX_MASK;
#define ADDRESS_TAG(addr)       ((addr & ADDRESS_TAG_MASK) >> 18) //TODO make configurable (also 6)
#define ADDRESS_OFFSET(addr)    ((addr & ADDRESS_OFFSET_MASK))
#define ADDRESS_INDEX(addr)     ((addr & ADDRESS_INDEX_MASK) >> 6)

struct CacheLine {
	uint64_t tag;
	uint8_t state;
	uint8_t lru;
};

struct statechange {
	int new_state;
	int bus_request;
};


typedef struct CacheLine* CacheState;

struct CacheState {
	struct CacheState* parent_cache;
	struct Memory* memory;
	size_t amount_children;
	size_t cur_size_children_array;
	struct CacheState** children; // List of children
	bool write_back;
	//TODO incorportate placement policy
	size_t size; // Size in lines //TODO during setup verify this is a multiple of child caches
	size_t line_size; // Cache line size in bytes
	//TODO implement different caches for data and instructions
};


// Sets up a new cache state with the following parameters
// - parent: is the higher level cache of the new cache, the new cache will be added to the list of children in the parent (memory should be NULL)
// - memory: if the cache is a LLC the parent should be null and memory should be set
// - write_back: true if cache is write back, false if cache is write througn
// - size: the size of the cache in the amount of lines, the size of the parent should be a multiple of this
// - line_size: the cache line size in bytes
struct CacheState* setup_cache(struct CacheState* parent, struct Memory* memory, bool write_back, size_t size, size_t line_size);

void add_child(struct CacheState* parent, struct CacheState* child);


// Simulates an access to the cache, updates the cache state corresponginly and returns wether the cache the access hit the cache
bool perform_cache_access(CacheState state, uint64_t cpu, uint64_t address, bool write);

void init_cachestate_masks(int indexsize, int offsetsize);
struct statechange get_msi_state_change(int current_state, bool write);
struct statechange get_msi_state_change_by_bus_request(int current_state, int bus_request);
int get_line_state(CacheState state, uint64_t cpu, uint64_t address, uint64_t* line_index);



#endif /* STATE_H */
