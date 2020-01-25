#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "config.h"
#include "memory.h"


struct Memory* init_memory(FILE* backing_file) {
	struct Memory* mem = malloc(sizeof(struct Memory));
	mem->table = calloc(1024, sizeof(struct Memory*));
	// mem->table = calloc(512, sizeof(struct Memory*));
	if(mem->table == NULL){
		printf("Could not calloc new simulated memory!\n");
		exit(1);
	}
	mem->backing_file = backing_file;
	return mem;
}

struct Memory* init_last_level_memory(FILE* backing_file, uint64_t address) {
	struct Memory* mem = malloc(sizeof(struct Memory));
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
		printf("Mem is null!\n");
		return 0;
	}
	if(address >= (1ULL<<42)) {
		printf("trying to access too large physaddre!%lx\n", address);
		return 0;
	}
	// if(address < mem->range_start || address > mem->range_end) {
	// 	printf("not in range!\n");
	// 	return 0;
	// }
	if((address & 0xFFF) + size > (1<<12)){
			debug_printf("Reading cross border!\n", 0);
			size_t left_size = (1 << 12 ) - (address & 0xFFF);
			size_t right_size = size - left_size;
	debug_printf("Splitting read, left:%x, right:%x\n", left_size, right_size);
			return read_sim_memory(mem, address, left_size, value)
	+ read_sim_memory(mem, address+left_size, right_size, ((uint8_t*)value)+left_size);
	}

	//L1 (bits 41:32)
	struct Memory* l1mem = find_in_level(mem, address, (address >> 32) & 0x3FF, false);

	//L3 (bits 31:22)
	struct Memory* l2mem = find_in_level(l1mem, address, (address >> 22) & 0x3FF, false);

	//L4 (bits 21:12)
	struct Memory* l3mem = find_in_level(l2mem, address, (address >> 12) & 0x3FF, true);
	memcpy(value, ((uint8_t*)l3mem->table) + (address & 0xFFF), size);
	return size;
}



int write_sim_memory(struct Memory* mem, uint64_t address, size_t size, void* value) {
	if(address >= (1ULL<<42)) {
		printf("Something is going wrong!%lx\n", address);
		return 0;
	}
	// if(address < mem->range_start || mem->range_end < address) {
	// 	printf("not in range!\n");
	// 	return 0;
	// }
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

	//L1 (bits 41:32)
	struct Memory* l1mem = find_in_level(mem, address, (address >> 32) & 0x3FF, false);

	//L3 (bits 31:22)
	struct Memory* l2mem = find_in_level(l1mem, address, (address >> 22) & 0x3FF, false);

	//L4 (bits 21:12)
	struct Memory* l3mem = find_in_level(l2mem, address, (address >> 12) & 0x3FF, true);
	memcpy(((void*)l3mem->table) + (address & 0xFFF), value, size);
	return size;

}
