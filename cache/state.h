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


// Simulates an access to the cache, updates the cache state corresponginly and returns wether the cache the access hit the cache
bool perform_cache_access(CacheState state, uint64_t cpu, uint64_t address, bool write);

void init_cachestate_masks(int indexsize, int offsetsize);
struct statechange get_msi_state_change(int current_state, bool write);
struct statechange get_msi_state_change_by_bus_request(int current_state, int bus_request);
int get_line_state(CacheState state, uint64_t cpu, uint64_t address, uint64_t* line_index);



#endif /* STATE_H */
