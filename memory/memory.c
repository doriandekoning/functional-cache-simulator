#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "config.h"
#include "memory.h"

#define MULTI_LEVEL_MEMORY 1

#ifdef MULTI_LEVEL_MEMORY
struct Memory* init_memory(FILE* backing_file) {
	struct Memory* mem = malloc(sizeof(struct Memory));
	mem->table = calloc(1<<14, sizeof(struct Memory*));
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
	mem->table = calloc(4096, sizeof(struct Memory*));
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
	// struct Memory* l1mem = find_in_level(mem, address, (address >> 32) & 0x3FF, false);

	//L3 (bits 31:22)
	struct Memory* l1mem = find_in_level(mem, address, (address >> 26) & 0x3FFF, false);

	//L4 (bits 21:12)
	struct Memory* l2mem = find_in_level(l1mem, address, (address >> 12) & 0x3FFF, true);
	memcpy(value, ((uint8_t*)l2mem->table) + (address & 0xFFF), size);
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
	// struct Memory* l1mem = find_in_level(mem, address, (address >> 40) & 0x3FF, false);

	//L3 (bits 31:22)
	struct Memory* l1mem = find_in_level(mem, address, (address >> 26) & 0x3FFF, false);

	//L4 (bits 21:12)
	struct Memory* l2mem = find_in_level(l1mem, address, (address >> 12) & 0x3FFF, true);
	memcpy(((void*)l2mem->table) + (address & 0xFFF), value, size);
	return size;

}


uint64_t get_size(struct Memory* mem) {
	uint64_t size = (1<<14) / (4096);
	for(int i = 0; i < (1<<14); i++) {
		struct Memory* first_level_entry = mem->table[i];
		if(first_level_entry != NULL ){
			size+=(1<<14) / (4096);
			for(int j = 0; j < (1<<14); j++) {
				struct Memory* second_level_entry = first_level_entry->table[j];
				if(second_level_entry != NULL) {
					size++;//(1<<14) / (4096);
					// for(int k = 0; k < 1024; k++) {
					// 	struct Memory* last_level_entry = second_level_entry->table[k];
					//  	if(last_level_entry != NULL) {
					//  		size++;
					//  	}
					// }
				}
			}
		}
	}
	return size;
}
#else

struct Memory* init_memory(FILE* backing_file) {
	struct Memory* mem = malloc(sizeof(struct Memory));
	printf("allocating mem!\n");
	mem->table = malloc(0x23fffffff); // Allocate a 1gb slab
	printf("Allocated mem!\n");
	if(mem->table == NULL){
		printf("Could not calloc new simulated memory!\n");
		exit(1);
	}
	mem->backing_file = backing_file;
	for(uint64_t i = 0; i < (0x23fffffff/1024); i++){
		if(fread(((uint8_t*)mem->table) + (i*1024), 1024, 1, backing_file) != 1) {
			printf("Unable to read from backing file!%lx\n", mem->table + (i*1024));
			return NULL;
		}
	}
	printf("Read complete memory from backing file!\n");
	return mem;
}

int write_sim_memory(struct Memory* mem, uint64_t address, size_t size, void* value) {
	if(address >= 0x23fffffff) {
		printf("Access out of bounds");
	}
	memcpy(((uint8_t*)mem->table) + (address), value, size);
	return size;
}

int read_sim_memory(struct Memory* mem, uint64_t address, size_t size, void* value) {
	if(address >= 0x23fffffff) {
		printf("Access out of bounds");
	}
	memcpy(value, ((uint8_t*)mem->table) + address, size);
	return size;
}


uint64_t get_size(struct Memory* mem) {
	return 1024*1024*1024;
}
#endif
