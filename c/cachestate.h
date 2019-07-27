#ifndef CACHESTATE_H
#define CACHESTATE_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"


extern const int STATE_INVALID;
extern const int STATE_INVALID;
extern const int STATE_SHARED;
extern const int STATE_MODIFIED;

extern const int BUS_REQUEST_NOTHING;
extern const int BUS_REQUEST_READ;
extern const int BUS_REQUEST_READX;
extern const int BUS_REQUEST_UPGR;
extern const int BUS_REQUEST_FLUSH;

struct CacheEntry {
	uint64_t address; //TODO rename cacheline
	int state;
	struct CacheEntry* next;
	struct CacheEntry* prev;
};

struct statechange {
	int new_state;
	int bus_request;
};

typedef struct access_s {
	uint64_t address;
	uint64_t tick;
	uint64_t cpu;
	bool write;
} access;


typedef struct CacheEntry* CacheEntryState;
typedef struct CacheEntry* CacheSetState;
typedef struct CacheEntry** CacheState;


uint64_t get_cache_set_number(uint64_t cache_line);
CacheSetState get_cache_set_state(CacheState state, uint64_t cache_line, uint64_t cpu);
CacheEntryState get_cache_entry_state(CacheSetState state, uint64_t cache_line);
void set_cache_set_state(CacheState state, CacheSetState new, uint64_t cache_line, uint64_t cpu);
struct statechange get_msi_state_change(int current_state, bool write);
struct statechange get_msi_state_change_by_bus_request(int current_state, int bus_request);
CacheSetState apply_state_change(CacheSetState cacheSetState, struct CacheEntry* entry, struct statechange statechange, uint64_t cache_line);


// Functions on a cachelinestate
void remove_item(struct CacheEntry* entry);
CacheSetState append_item(CacheSetState list, struct CacheEntry* newEntry);
void move_item_back(struct CacheEntry* entry);
int list_length(CacheSetState list);
struct CacheEntry* get_head(struct CacheEntry* list);

#endif /* CACHESTATE_H */
