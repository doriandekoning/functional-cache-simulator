#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "pagetable.h"


#define PG_PRESENT_MASK		(1 << 0)
#define PG_PSE_MASK		(1 << 7)
#define INDEX_MASK		0x1ff

uint64_t vaddr_to_phys(pagetable* table, uint64_t virtaddress) {
	uint64_t l0_index = (virtaddress >> 48);
	uint64_t l1_index = (virtaddress >> 39) & INDEX_MASK;
	uint64_t l2_index = (virtaddress >> 30) & INDEX_MASK;
	uint64_t l3_index = (virtaddress >> 21) & INDEX_MASK;
	uint64_t l4_index = (virtaddress >> 12) & INDEX_MASK;

	//L1
	uint64_t l1_phys = table->physical_addresses[l1_index];
	if(!(l1_phys & PG_PRESENT_MASK)) {
		return 0; //Not found
	}
	pagetable* l1_entry = table->next_levels[l1_index];
	//L2
	uint64_t l2_phys = l1_entry->physical_addresses[l2_index];
	if(!(l2_phys & PG_PRESENT_MASK)) {
		return 0; //Not found
	}
	if(l2_phys & PG_PSE_MASK) {
		// 1G page found
		return  (l2_phys & 0xFFFFC0000000);
	}
	pagetable* l2_entry = l1_entry->next_levels[l2_index];

	//L3
	uint64_t l3_phys = l2_entry->physical_addresses[l3_index];
	if(!(l3_phys & PG_PRESENT_MASK)) {
		return 0; //Not found
	}
	if(l3_phys & PG_PSE_MASK) {
		// 2M page
		return l3_phys & 0xFFFFFFE00000;
	}

	pagetable* l3_entry = l2_entry->next_levels[l3_index];
	uint64_t l4_phys = l3_entry->physical_addresses[l4_index];
	if(!(l4_phys & PG_PRESENT_MASK)){
		return 0; //Not found
	}
	return  l4_phys & 0xFFFFFFFFF000;
}

pagetable* init_pagetable() {
	pagetable* ret = malloc(sizeof(pagetable));
	ret->physical_addresses = calloc(512, sizeof(uint64_t));
	ret->next_levels = calloc(512, sizeof(pagetable*));
	return ret;
}

pagetable* read_pagetable(char* path, uint64_t* cr3) {
	FILE* f = fopen(path, "r");
	uint64_t totalLevelsRead = 0;
	if(!f) {
		return NULL;
	}
	if(fread(cr3, sizeof(uint64_t), 1, f) != 1) {
		printf("Could not read cr3!\n");
		return NULL;
	}
	pagetable* l1 = init_pagetable();
	if(fread(l1->physical_addresses, sizeof(uint64_t), 512, f) != 512) {
		printf("Could not read L1");
		return NULL;
	}
	totalLevelsRead++;
	for(int l1i = 0; l1i < 512; l1i ++ ) {
		if(l1->physical_addresses[l1i] & PG_PRESENT_MASK) {
			//Setup new table
			pagetable* l2 = init_pagetable();
			l1->next_levels[l1i] = l2;
			if(fread(l2->physical_addresses, sizeof(uint64_t), 512, f) != 512) {
				printf("Could not read l2\n");
				return NULL;
			}
			totalLevelsRead++;
			for(int l2i = 0; l2i < 512; l2i++ ) {
				if(l2->physical_addresses[l2i] & PG_PRESENT_MASK && !(l2->physical_addresses[l2i] & PG_PSE_MASK)) {
					//Setup new table
					pagetable* l3 = init_pagetable();
					l2->next_levels[l2i] = l3;
					if(fread(l3->physical_addresses, sizeof(uint64_t), 512, f) != 512) {
						printf("Could not read l3\n");
						return NULL;
					}
					totalLevelsRead++;
					for(int l3i = 0; l3i < 512; l3i++) {
						if(l3->physical_addresses[l3i] & PG_PRESENT_MASK && !(l3->physical_addresses[l3i] & PG_PSE_MASK)) {
							pagetable* l4 = init_pagetable();
							l3->next_levels[l3i] = l4;
							if(fread(l4->physical_addresses, sizeof(uint64_t), 512, f) != 512) {
								printf("Could not read l4, l1: %d,l2: %d,l3: %d, %lu\n", l1i, l2i, l3i, totalLevelsRead);
								return NULL;
							}
							totalLevelsRead++;

						}
					}
				}
			}

		}
	}
	printf("Total subtables:%d\n", totalLevelsRead);
	return l1;
}
