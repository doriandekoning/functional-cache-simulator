#ifndef __PAGETABLE_H
#define __PAGETABLE_H

#include <stdint.h>

typedef struct pagetable pagetable;
struct pagetable {
	uint64_t* physical_addresses;
	pagetable** next_levels;
};


uint64_t vaddr_to_phys(pagetable* table, uint64_t virtaddress);
pagetable* read_pagetable(char* path, uint64_t* cr3);
#endif
