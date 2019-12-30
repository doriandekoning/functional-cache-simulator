#ifndef STATE_H
#define STATE_H
#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "pipereader.h"

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

struct CacheEntry {
	uint64_t tag;
	int state;
	struct CacheEntry* next;
	struct CacheEntry* prev;
};

struct statechange {
	int new_state;
	int bus_request;
};


typedef struct CacheEntry* CacheEntryState;
typedef struct CacheEntry* CacheSetState;
typedef struct CacheEntry** CacheState;


// Simulates an access to the cache, updates the cache state corresponginly and returns wether the cache the access hit the cache
bool perform_cache_access(CacheState state, uint64_t cpu, uint64_t address, bool write);

void init_cachestate_masks(int indexsize, int offsetsize);
uint64_t get_index(uint64_t address);
CacheSetState get_cache_set_state(CacheState state, uint64_t address, uint64_t cpu);
CacheEntryState get_cache_entry_state(CacheSetState state, uint64_t address);
void set_cache_set_state(CacheState state, CacheSetState new, uint64_t address, uint64_t cpu);
struct statechange get_msi_state_change(int current_state, bool write);
struct statechange get_msi_state_change_by_bus_request(int current_state, int bus_request);
CacheSetState apply_state_change(CacheSetState cacheSetState, struct CacheEntry* entry, struct statechange statechange, uint64_t address);


// Functions on a cachelinestate
void remove_item(struct CacheEntry* entry);
CacheSetState append_item(CacheSetState list, struct CacheEntry* newEntry);
void move_item_back(struct CacheEntry* entry);
int list_length(CacheSetState list);
struct CacheEntry* get_head(struct CacheEntry* list);

#endif /* STATE_H */
