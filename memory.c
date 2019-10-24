#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "config.h"
#include "memory.h"




uint64_t total_subtables = 0;

struct memory* init_memory() {
        struct memory* mem = malloc(sizeof(struct memory));
	total_subtables++;
	mem->table = calloc(512, 8);
	if(mem->table == NULL){
		printf("Could not calloc new simulated memory!\n");
		exit(1);
	}
        return mem;
}

struct memory* init_last_level_memory() {
        struct memory* mem = malloc(sizeof(struct memory));
        total_subtables++;
        mem->table = calloc(4096, 8);
        if(mem->table == NULL){
		printf("Could not calloc new simulated memory!\n");
		exit(1);
        }
        return mem;
}

void free_memory(struct memory* mem) {
	//TODO free with dfs
}
struct memory* find_in_level(struct memory* mem, uint64_t level_addr, bool lastlevel) {
	if(mem->table[level_addr ] == NULL) {
		//printf("Not found in level!\n");
		if(lastlevel) {
			mem->table[level_addr ] = init_last_level_memory();
		}else{
			mem->table[level_addr ] = init_memory();
		}
	}
	return mem->table[level_addr];
}

int read_sim_memory(struct memory* mem, uint64_t address, size_t size, void* value) {
//	printf("Ring at %lx : 0x%x bytes\n", address, size);
	if(mem == NULL){
		return 0;
	}
        if((address & 0xFFF) + size > (1<<12)){
                printf("Reading cross border!%d\n");
                size_t left_size = (1 << 12 ) - (address & 0xFFF);
                size_t right_size = size - left_size;
		printf("Splitting read, left:%x, right:%x\n", left_size, right_size);
                return read_sim_memory(mem, address, left_size, value)
		+ read_sim_memory(mem, address+left_size, right_size, value);
        }
	struct memory* l1mem = find_in_level(mem, (address >>47) & 0x1, false);
	//L1 (bits 48:39)
        debug_printf("RL1: 0x%lx\n", (address >> 39) & 0x1FF);
        struct memory* l2mem = find_in_level(l1mem, (address >> 39) & 0x1FF, false);

        //L2 (bits 38:30)
	debug_printf("RL2: 0x%lx\n", (address >> 30) & 0x1FF);
        struct memory* l3mem = find_in_level(l2mem, (address >> 30) & 0x1FF, false);

        //L3 (bits 29:21)
	debug_printf("RL3: 0x%lx\n", (address >> 21) & 0x1FF);
        struct memory* l4mem = find_in_level(l3mem, (address >> 21) & 0x1FF, false);

        //L4 (bits 20:12)
	debug_printf("RL4: 0x%lx\n", (address >> 12) & 0x1FF);
        struct memory* l5mem = find_in_level(l4mem, (address >> 12) & 0x1FF, true);



//	printf("RL5: 0x%lx\n", address & 0xFFF);
	//printf("Reading 5 at:%lx\n", level_idx5);
//	printf("Writing: %p, %p\n", l4mem->table,  ((uint8_t*)l4mem->table) + level_idx5);
        memcpy(value, ((uint8_t*)l5mem->table) + (address & 0xFFF), size);
        return size;
}



int write_sim_memory(struct memory* mem, uint64_t address, size_t size, void* value) {
	if((address & 0xFFF) + size > (1<<12)){
	//TODO check how often used
		size_t left_size = (1 << 12 ) - (address & 0xFFF);
		size_t right_size = size - left_size;
		debug_printf("Writing left:\t%p, %lx, %d, %p\n", mem, address, left_size, value);
		debug_printf("Writing right:\t%p, %lx, %d, %p\n", mem, address+left_size, right_size, value+left_size);
		int written_left = write_sim_memory(mem, address, left_size, value);
		int written_right = write_sim_memory(mem, address+left_size, right_size, value);
		return written_left + written_right;
	}
	debug_printf("WL1:0x%03lx\n",  (address >> 47) & 0x1);
	struct memory* l1mem = find_in_level(mem, (address >> 47) & 0x1, false);
	//L1 (bits 48:39)
	debug_printf("WL2:0x%03lx\n",  (address >> 39) & 0x1FF);
	struct memory* l2mem = find_in_level(l1mem, (address >> 39) & 0x1FF, false);

	//L2 (bits 38:30)
	debug_printf("WL3:0x%03lx\n",  (address >> 30) & 0x1FF);
	struct memory* l3mem = find_in_level(l2mem, (address >> 30) & 0x1FF, false);

	//L3 (bits 29:21)
	debug_printf("WL4:0x%03lx\n",  (address >> 21) & 0x1FF);
	struct memory* l4mem = find_in_level(l3mem, (address >> 21) & 0x1FF, false);

	//L4 (bits 20:12)
	debug_printf("WL5:0x%03lx\n",  (address >> 12) & 0x1FF);
	struct memory* l5mem = find_in_level(l4mem, (address >> 12) & 0x1FF, true);
	memcpy(((void*)l5mem->table) + (address & 0xFFF), value, size);
	return size;

}


