#ifndef __PAGETABLE_H
#define __PAGETABLE_H

#include <stdint.h>


extern uint64_t nextfreephys;

typedef struct pagetable_entry_t {
	void* next;
} pagetable_entry;

typedef struct pagetable_t {
	pagetable_entry* root;
	uint64_t levels;
	uint64_t addressLength;
} pagetable;


static inline uint64_t bits_per_level(pagetable* table) {
	return table->addressLength/table->levels;
}

static inline uint64_t pages_per_level(pagetable* table) {
	return ((uint64_t)1 << bits_per_level(table) );
}

static inline uint64_t get_level_tag(pagetable* table, uint64_t address, uint64_t level) {
	uint64_t bpl = bits_per_level(table);
	return ((address >> (uint64_t)12) >> (level*bpl)) % ((uint64_t)1 << bpl);
}


pagetable_entry* find_in_level(pagetable* tablee, pagetable_entry* level, uint64_t address);
pagetable_entry* setup_pagetable_entry();
uint64_t vaddr_to_phys(pagetable* table, uint64_t virtaddress);

#endif
