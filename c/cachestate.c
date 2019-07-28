#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "cachestate.h"


const int STATE_INVALID = 0;
const int STATE_SHARED = 1;
const int STATE_MODIFIED = 2;

const int BUS_REQUEST_NOTHING = 0;
const int BUS_REQUEST_READ = 1;
const int BUS_REQUEST_READX = 2;
const int BUS_REQUEST_UPGR = 3;
const int BUS_REQUEST_FLUSH = 4;

uint64_t get_cache_set_number(uint64_t cache_line) {
	return cache_line % AMOUNT_CACHE_SETS;
}

CacheSetState get_cache_set_state(CacheState state, uint64_t cache_line, uint64_t cpu) {
	return state[(AMOUNT_CACHE_SETS * cpu) + get_cache_set_number(cache_line)];
}

void set_cache_set_state(CacheState state, CacheSetState new, uint64_t cache_line, uint64_t cpu) {
	state[(AMOUNT_CACHE_SETS * cpu)  + get_cache_set_number(cache_line)] = (struct CacheEntry*)new;
}


// Returns the cacheEntry assosciated with the given address or NULL if not present in the cache
CacheEntryState get_cache_entry_state(CacheSetState cacheSetState, uint64_t cache_line) {
	CacheEntryState next = cacheSetState;
	while(next != NULL) {
		if(next->address == cache_line) {
			return next;
		}
		next = next->next;
	}
	return NULL;
}


struct statechange get_msi_state_change(int current_state, bool write) {
	struct statechange ret;
	if(write) {
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
	} else{
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



CacheSetState apply_state_change(CacheSetState cacheLineState, struct CacheEntry* entry, struct statechange statechange, uint64_t cache_line) {
	struct CacheEntry* head = get_head(cacheLineState);
	if(entry != NULL && statechange.new_state == STATE_INVALID) {
		//Remove line from cache
		if(entry == head){
			head = entry->next;
		}
		remove_item(entry);
		free(entry);

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
		new->address = cache_line;
		new->state = statechange.new_state;

		cacheLineState = append_item(cacheLineState, new);
		get_head(cacheLineState);
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

