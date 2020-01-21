#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "config.h"
#include "memory.h"


uint64_t total_subtables = 0;

struct Memory* init_memory(FILE* backing_file) {
	struct Memory* mem = malloc(sizeof(struct Memory));
	total_subtables++;
	mem->table = calloc(512, sizeof(struct Memory*));
	if(mem->table == NULL){
		printf("Could not calloc new simulated memory!\n");
		exit(1);
	}
	mem->backing_file = backing_file;
	return mem;
}

struct Memory* init_last_level_memory(FILE* backing_file, uint64_t address) {
	struct Memory* mem = malloc(sizeof(struct Memory));
	total_subtables++;
	mem->table = malloc(4096);
	if(mem->table == NULL){
		printf("Could not calloc new simulated memory!\n");
		exit(1);
	}
	//Load from backing file
	if(backing_file) {
		mem->backing_file = backing_file;
		fseek(mem->backing_file, address & ~(0xFFFULL), SEEK_SET);
		if(fread(mem->table, 4096, 1, backing_file) != 1) {
			printf("Unable to read from backing file!%lx\n", address & ~(0xFFFULL));
			return NULL;
		}
	}
	return mem;
}

void free_memory(struct Memory* mem) {
	//TODO free with dfs
}
struct Memory* find_in_level(struct Memory* mem, uint64_t address, uint64_t level_addr, bool lastlevel) {
	if(mem->table[level_addr ] == NULL) {
		//printf("Not found in level!\n");
		if(lastlevel) {
			mem->table[level_addr ] = init_last_level_memory(mem->backing_file, address);
		}else{
			mem->table[level_addr ] = init_memory(mem->backing_file);
		}
	}
	return mem->table[level_addr];
}

int read_sim_memory(struct Memory* mem, uint64_t address, size_t size, void* value) {
	if(mem == NULL){
		return 0;
	}
	if((address & 0xFFF) + size > (1<<12)){
			debug_printf("Reading cross border!\n", 0);
			size_t left_size = (1 << 12 ) - (address & 0xFFF);
			size_t right_size = size - left_size;
	debug_printf("Splitting read, left:%x, right:%x\n", left_size, right_size);
			return read_sim_memory(mem, address, left_size, value)
	+ read_sim_memory(mem, address+left_size, right_size, ((uint8_t*)value)+left_size);
	}
	struct Memory* l1mem = find_in_level(mem, address, (address >>47) & 0x1, false);
	//L1 (bits 48:39)
	//      debug_printf("RL1: 0x%lx\n", (address >> 39) & 0x1FF);
	struct Memory* l2mem = find_in_level(l1mem, address, (address >> 39) & 0x1FF, false);

	//L2 (bits 38:30)
	//	debug_printf("RL2: 0x%lx\n", (address >> 30) & 0x1FF);
	struct Memory* l3mem = find_in_level(l2mem, address, (address >> 30) & 0x1FF, false);

	//L3 (bits 29:21)
	//	debug_printf("RL3: 0x%lx\n", (address >> 21) & 0x1FF);
	struct Memory* l4mem = find_in_level(l3mem, address, (address >> 21) & 0x1FF, false);

	//L4 (bits 20:12)
	//debug_printf("RL4: 0x%lx\n", (address >> 12) & 0x1FF);
	struct Memory* l5mem = find_in_level(l4mem, address, (address >> 12) & 0x1FF, true);

//	printf("RL5: 0x%lx\n", address & 0xFFF);
	//printf("Reading 5 at:%lx\n", level_idx5);
//	printf("Writing: %p, %p\n", l4mem->table,  ((uint8_t*)l4mem->table) + level_idx5);
	memcpy(value, ((uint8_t*)l5mem->table) + (address & 0xFFF), size);
	return size;
}



int write_sim_memory(struct Memory* mem, uint64_t address, size_t size, void* value) {
	if((address & 0xFFF) + size > (1<<12)){
		size_t left_size = (1 << 12 ) - (address & 0xFFF);
		size_t right_size = size - left_size;
		debug_printf("Splitting write: %016lx, left: %d, right: %d!\n", address, left_size, right_size);

		debug_printf("Writing left:\t%p, %lx, %d, %p\n", mem, address, left_size, value);
		debug_printf("Writing right:\t%p, %lx, %d, %p\n", mem, address+left_size, right_size, ((uint8_t*)value)+left_size);
		int written_left = write_sim_memory(mem, address, left_size, value);
		int written_right = write_sim_memory(mem, address+left_size, right_size, ((uint8_t*)value)+left_size);
		return written_left + written_right;
	}
	//debug_printf("WL1:0x%03lx\n",  (address >> 47) & 0x1);
	struct Memory* l1mem = find_in_level(mem, address, (address >> 47) & 0x1, false);
	//L1 (bits 48:39)
	//debug_printf("WL2:0x%03lx\n",  (address >> 39) & 0x1FF);
	struct Memory* l2mem = find_in_level(l1mem, address, (address >> 39) & 0x1FF, false);

	//L2 (bits 38:30)
	//debug_printf("WL3:0x%03lx\n",  (address >> 30) & 0x1FF);
	struct Memory* l3mem = find_in_level(l2mem, address, (address >> 30) & 0x1FF, false);

	//L3 (bits 29:21)
	//debug_printf("WL4:0x%03lx\n",  (address >> 21) & 0x1FF);
	struct Memory* l4mem = find_in_level(l3mem, address, (address >> 21) & 0x1FF, false);

	//L4 (bits 20:12)
	//debug_printf("WL5:0x%03lx\n",  (address >> 12) & 0x1FF);
	struct Memory* l5mem = find_in_level(l4mem, address, (address >> 12) & 0x1FF, true);
	memcpy(((void*)l5mem->table) + (address & 0xFFF), value, size);
	return size;

}




