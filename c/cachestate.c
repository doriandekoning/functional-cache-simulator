#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "cachestate.h"

uint64_t ADDRESS_OFFSET_MASK = 0;
uint64_t ADDRESS_TAG_MASK = 0;
uint64_t ADDRESS_INDEX_MASK = 0;
uint64_t setupmask(unsigned from, unsigned to) {
        uint64_t mask = 0;
        for(unsigned i = from; i <=to; i++) {
                mask |= (1ULL << i);
        }
        return mask;
}

const int STATE_INVALID = 0;
const int STATE_SHARED = 1;
const int STATE_MODIFIED = 2;

const int BUS_REQUEST_NOTHING = 0;
const int BUS_REQUEST_READ = 1;
const int BUS_REQUEST_READX = 2;
const int BUS_REQUEST_UPGR = 3;
const int BUS_REQUEST_FLUSH = 4;


void init_cachestate_masks(int indexsize, int offsetsize) {
        ADDRESS_OFFSET_MASK = setupmask(0,(offsetsize-1));
        ADDRESS_TAG_MASK = setupmask(offsetsize+indexsize,63);
        ADDRESS_INDEX_MASK = setupmask(offsetsize,(indexsize+offsetsize-1));
}

CacheSetState get_cache_set_state(CacheState state, uint64_t address, uint64_t cpu) {
	return state[(AMOUNT_CACHE_SETS * cpu) + ADDRESS_INDEX(address)];
}

void set_cache_set_state(CacheState state, CacheSetState new, uint64_t address, uint64_t cpu) {
	state[(AMOUNT_CACHE_SETS * cpu)  + ADDRESS_INDEX(address)] = (struct CacheEntry*)new;
}


// Returns the cacheEntry assosciated with the given address or NULL if not present in the cache
CacheEntryState get_cache_entry_state(CacheSetState cacheSetState, uint64_t address) {
	CacheEntryState next = cacheSetState;
	while(next != NULL) {
		if(next->tag == ADDRESS_TAG(address)) {
			return next;
		}
		next = next->next;
	}
	return NULL;
}


struct statechange get_msi_state_change(int current_state, uint8_t access_type) {
	struct statechange ret;
	if(access_type == CACHE_WRITE) {
		if(current_state == STATE_INVALID) {
			//PrWrI: accessing processor to M and issues BusRdX
			ret.new_state = STATE_MODIFIED;
			ret.bus_request = BUS_REQUEST_READX;
		} else if(current_state == STATE_SHARED) {
			//PrWrS: that processor to M others to I
			ret.new_state = STATE_MODIFIED;
			ret.bus_request = BUS_REQUEST_UPGR;
		} else if(current_state == STATE_MODIFIED) {
			//PrWrM: accessing processor to M others stays same
			ret.new_state = STATE_MODIFIED;
			ret.bus_request = BUS_REQUEST_NOTHING;
		} else {
			ret.new_state = -1;
			ret.bus_request = -1;
		}
	} else if( access_type == CACHE_READ) {
		if(current_state == STATE_INVALID){
			//PrRdI: accessing processor to S others to S
			ret.new_state = STATE_SHARED;
			ret.bus_request = BUS_REQUEST_READ;
		} else if(current_state == STATE_SHARED){
			ret.new_state = STATE_SHARED;
			ret.bus_request = BUS_REQUEST_NOTHING;
		} else if(current_state == STATE_MODIFIED){
			ret.new_state = STATE_MODIFIED;
			ret.bus_request = BUS_REQUEST_NOTHING;
		} else {
			ret.new_state = -1;
			ret.bus_request = -1;
		}
	}else{
		printf("Received unknown access type!\n");
		exit(-1);
	}
	return ret;
}

struct statechange get_msi_state_change_by_bus_request(int current_state, int bus_request){
	struct statechange ret;
	ret.bus_request = BUS_REQUEST_NOTHING;
	if(current_state == STATE_INVALID) {
		ret.new_state = STATE_INVALID;
	} else if(current_state == STATE_SHARED) {
		if(bus_request == BUS_REQUEST_READ) {
			ret.new_state = STATE_SHARED;
		} else {
			ret.new_state = STATE_INVALID;
		}
	} else if(current_state == STATE_MODIFIED) {
		ret.bus_request = BUS_REQUEST_FLUSH;
		if(bus_request == BUS_REQUEST_READ) {
			ret.new_state = STATE_SHARED;
		} else {
			ret.new_state = STATE_INVALID;
		}
	} else{
		printf("State transition unknown!\n");
		exit(7);
	}
	return ret;
}



CacheSetState apply_state_change(CacheSetState cacheLineState, struct CacheEntry* entry, struct statechange statechange, uint64_t address) {
	struct CacheEntry* head = get_head(cacheLineState);
	if(entry != NULL && statechange.new_state == STATE_INVALID) {
		//Remove line from cache
		if(entry == head){
			head = entry->next;
		}
		remove_item(entry);
		free(entry);
		return head;
	} else if (entry != NULL) {
		// Move entry to back (front is LRU)
		if(entry == head) {
			head = entry->next;
		}
		entry->state = statechange.new_state;
		move_item_back(entry);

	} else {
		// Add new entry to cache and evict oldest one if more than 8
		struct CacheEntry* new = calloc(1, sizeof(struct CacheEntry));
		if(new == NULL) {
			printf("Could not allocate new cacheentry\n");
			exit(1);
		}
		new->tag = ADDRESS_TAG(address);
		new->state = statechange.new_state;

		cacheLineState = append_item(cacheLineState, new);
	}
	return get_head(cacheLineState); //TODO optimize
}


void remove_item(struct CacheEntry* entry) {
	if(entry->prev != NULL) {
		entry->prev->next = entry->next;
	}
	if(entry->next != NULL) {
		entry->next->prev = entry->prev;
	}
}

CacheSetState append_item(CacheSetState list, struct CacheEntry* newEntry) {
	if(list == NULL){
		return newEntry;
	}
	CacheSetState next = list;
	while(next->next != NULL) {
		next = next->next;
	}
	next->next = newEntry;
	newEntry->prev = next;
	newEntry->next = NULL;
	return list;
}

void move_item_back(struct CacheEntry* entry) {
	struct CacheEntry* end = entry;
	while(end->next != NULL) {
		end = end->next;
	}
	if(end != entry) {
		if(entry->prev != NULL) {
			entry->prev->next = entry->next;
		}
		if(entry->next != NULL) {
			entry->next->prev = entry->prev;
		}
		end->next = entry;
		entry->prev = end;
		entry->next = NULL;
	}
}

int list_length(struct CacheEntry* list) {
	struct CacheEntry* next = list;
	int size = 0;
	while(next != NULL){
		size++;
		next = next->next;
	}
	if(list == NULL || list->prev == NULL) {
		return size;
	}
	struct CacheEntry* prev = list->prev;
	while(prev != NULL) {
		size++;
		prev = prev->prev;
	}
	return size;
}

struct CacheEntry* get_head(struct CacheEntry* list) {
	struct CacheEntry* head = list;
	while(head != NULL && head->prev != NULL) {
		head = head->prev;
	}
	return head;
}

