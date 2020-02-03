#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "config.h"
#include "memory/memory_range.h"
#include "memory/memory.h"



struct MemoryRange* read_memory_ranges(FILE* memory_ranges_file) {
    debug_print("Reading memory ranges!\n");
	uint64_t amount;
	fscanf(memory_ranges_file, "%lx\n", &amount);
    struct MemoryRange* cur = NULL;
    debug_printf("%d memory ranges found!\n", amount);
	for(uint64_t i = 0; i < amount; i++) {
        debug_printf("Reading memory range:%d\n", i);
        struct MemoryRange* new = malloc(sizeof(struct MemoryRange));
        debug_printf("Allocated memory for memory range:%p\n", new);
        new->next = cur;
        cur = new;

        debug_printf("Scanf memory range %d\n", i);
        debug_printf("Testmalloc3:%p\n", malloc(1));
		fscanf(memory_ranges_file, "%lx-%lx\n", &(cur->start_addr), &(cur->end_addr));
        debug_printf("Testmalloc4:%p\n", malloc(1));
		printf("Memrange: [%lx - %lx]\n", cur->start_addr, cur->end_addr);
	}

	return cur;
}


struct Memory* get_memory_for_address(int amount_memories, struct Memory* memories, uint64_t address) {
    debug_printf("Finding memory for address:%lx\n", address);
    for(int i = 0; i < amount_memories; i++) {
        if(memories[i].addr_start <= address && address < memories[i].addr_end ) {
            debug_printf("Found memory for address:%d\n", i);
            return &(memories[i]);
        }
    }
    debug_printf("Memory for address not found!%lx\n", address);
    return NULL;
}
