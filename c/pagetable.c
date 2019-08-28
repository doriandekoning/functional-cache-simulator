#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "pagetable.h"

uint64_t nextfreephys = (1 << 12); //TODO make global or something


pagetable_entry* setup_pagetable_entry(uint64_t pages) {
	pagetable_entry* ret = calloc(pages, sizeof(pagetable_entry));
	if(ret == NULL ){
		printf("Cannot alloc!\n");
		exit(1);
	}
	return ret;
}

pagetable_entry* find_in_level(pagetable* table, pagetable_entry* level, uint64_t address) {
	pagetable_entry* entry = (pagetable_entry*)level[address].next;
	if(entry == NULL) {
		entry = setup_pagetable_entry(pages_per_level(table));
		level[address].next = entry;
	}
	return entry;
}

uint64_t vaddr_to_phys(pagetable* table, uint64_t virtaddress) {
	int curlevel = 0;
	if(table->root == NULL) {
		uint64_t pagesPerLevel = pages_per_level(table);
		table->root = setup_pagetable_entry(pagesPerLevel);
	}
	pagetable_entry* entry = table->root;
	//Search through layers (0, n-1)
	while(curlevel+1 < table->levels) {
		if(entry->next == NULL) {
			entry->next = setup_pagetable_entry(pages_per_level(table));
		}
		entry = find_in_level(table, (pagetable_entry*)entry->next, get_level_tag(table, virtaddress, curlevel));
		curlevel++;
	}


	if(entry[get_level_tag(table, virtaddress, curlevel)].next == 0){
		entry[get_level_tag(table, virtaddress, curlevel)].next = (void*)nextfreephys;
		nextfreephys += (1 << 12);
		return (uint64_t)entry[get_level_tag(table, virtaddress, curlevel)].next;
	}
	return (uint64_t)entry[get_level_tag(table, virtaddress, curlevel)].next;

}
