#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "config.h"
#include "memory.h"
#include "memory_range.h"

// #define MULTI_LEVEL_MEMORY 1

#ifdef MULTI_LEVEL_MEMORY
struct Memory* init_memory(FILE* backing_file, uint64_t addr_start, uint64_t addr_end) {
	struct Memory* mem = malloc(sizeof(struct Memory));
	mem->table = calloc(1<<14, sizeof(struct Memory*));
	if(mem->table == NULL){
		printf("Could not calloc new simulated memory!\n");
		exit(1);
	}
	mem->addr_start = addr_start;
	mem->addr_end = addr_end;
	mem->backing_file = backing_file;
	return mem;
}



struct Memory* init_last_level_memory(struct Memory* parent, uint64_t address) {
	struct Memory* mem = malloc(sizeof(struct Memory));
	mem->table = calloc(4096, sizeof(struct Memory*));
	mem->addr_start = parent->addr_start;
	mem->addr_end = parent->addr_end;
	if(mem->table == NULL){
		printf("Could not calloc new simulated memory!\n");
		exit(1);
	}
	debug_printf("Reading from memory:%lx, [%lx-%lx]\n", address, parent->addr_start, parent->addr_end);
	//Load from backing file
	if(parent->backing_file && mem->addr_start <= address && address < mem->addr_end) {
		mem->backing_file = parent->backing_file;
		fseek(mem->backing_file, (address-mem->addr_start) & ~(0xFFFULL), SEEK_SET);
		if(fread(mem->table, 4096, 1, mem->backing_file) != 1) {
			printf("Unable to read from backing file:%p addr %llx\n", parent->backing_file, (address-mem->addr_start) & ~(0xFFFULL));
			exit(1);
		}
	}else{
		printf("memory out of range: %lx <= %lx <= %lx or backing file not found\n", mem->addr_start, address, mem->addr_end);
		exit(1);
	}
	return mem;
}

void free_memory(struct Memory* mem) {
	//TODO free with dfs
	(void)mem;
}
struct Memory* find_in_level(struct Memory* mem, uint64_t address, uint64_t level_addr, bool lastlevel) {
	if(mem->table[level_addr] == NULL) {
		if(lastlevel) {
			mem->table[level_addr ] = init_last_level_memory(mem, address);
		}else{
			mem->table[level_addr ] = init_memory(mem->backing_file, mem->addr_start, mem->addr_end);
		}
	}
	return mem->table[level_addr];
}

int read_sim_memory(int amount_memories, struct Memory* memories, uint64_t address, size_t size, void* value) {
	debug_print("Reading sim memory!\n");
	struct Memory* mem = get_memory_for_address(amount_memories, memories, address);
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
			return read_sim_memory(amount_memories, memories, address, left_size, value)
	+ read_sim_memory(amount_memories, memories, address+left_size, right_size, ((uint8_t*)value)+left_size);
	}

	//L1 (bits 41:32)
	// struct Memory* l1mem = find_in_level(mem, address, (address >> 32) & 0x3FF, false);

	//L3 (bits 31:22)
	struct Memory* l1mem = find_in_level(mem, address, (address >> 26) & 0x3FFF, false);

	//L4 (bits 21:12)
	struct Memory* l2mem = find_in_level(l1mem, address, (address >> 12) & 0x3FFF, true);

	void* memory_loc =  ((uint8_t*)l2mem->table) + (address & 0xFFF);
	#ifdef USE_MEMCPY
	memcpy(value, memory_loc, size);
	#else
	if(memory_loc != NULL){
		if(size > 16) {
		}else if(size == 8) {
			*(uint64_t*)value = *(uint64_t*)memory_loc;
		}else if(size == 4) {
			*(uint32_t*)value = *(uint32_t*)memory_loc;
		}else if (size == 2) {
			*(uint16_t*)value = *(uint16_t*)memory_loc;
		}else if (size == 1) {
			*(uint8_t*)value = *(uint8_t*)memory_loc;
		}
	}
	#endif
	debug_print("Read sim memory!\n");
	return size;
}



int write_sim_memory(int amount_memories, struct Memory* memories, uint64_t address, size_t size, void* value) {
	debug_print("Writing sim memory!\n");
	struct Memory* mem = get_memory_for_address(amount_memories, memories, address);
	if(mem == NULL) {
		debug_print("Address not in memory!\n");
		return 0;
	}
	if((address & 0xFFF) + size > (1<<12)){
		size_t left_size = (1 << 12 ) - (address & 0xFFF);
		size_t right_size = size - left_size;
		debug_printf("Splitting write: %016lx, left: %d, right: %d!\n", address, left_size, right_size);

		debug_printf("Writing left:\t%p, %lx, %d, %p\n", mem, address, left_size, value);
		debug_printf("Writing right:\t%p, %lx, %d, %p\n", mem, address+left_size, right_size, ((uint8_t*)value)+left_size);
		int written_left = write_sim_memory(amount_memories, memories, address, left_size, value);
		int written_right = write_sim_memory(amount_memories, memories, address+left_size, right_size, ((uint8_t*)value)+left_size);
		return written_left + written_right;
	}

	//L1 (bits 41:32)
	// struct Memory* l1mem = find_in_level(mem, address, (address >> 40) & 0x3FF, false);

	//L3 (bits 31:22)
	struct Memory* l1mem = find_in_level(mem, address, (address >> 26) & 0x3FFF, false);

	//L4 (bits 21:12)
	struct Memory* l2mem = find_in_level(l1mem, address, (address >> 12) & 0x3FFF, true);
	void* memory_loc = ((void*)l2mem->table) + (address & 0xFFF);
	#ifdef USE_MEMCPY
	memcpy(memory_loc, value, size);
	#else
	if(memory_loc != NULL){
		if(size > 16) {
			// Intentionally do nothing
		}else if(size == 8) {
			*(uint64_t*)memory_loc = *(uint64_t*)value;
		}else if(size == 4) {
			*(uint32_t*)memory_loc = *(uint32_t*)value;
		}else if (size == 2) {
			*(uint16_t*)memory_loc = *(uint16_t*)value;
		}else if (size == 1) {
			*(uint8_t*)memory_loc = *(uint8_t*)value;
		}
	}
	#endif
	debug_print("Wrote sim memory!\n");
	return size;

}


uint64_t get_size(struct Memory* mem) {
	uint64_t size = (1<<14) / (4096) * sizeof(void*);
	for(int i = 0; i < (1<<14); i++) {
		struct Memory* first_level_entry = mem->table[i];
		if(first_level_entry != NULL ){
			size+=(1<<14) / (4096);
			for(int j = 0; j < (1<<14); j++) {
				struct Memory* second_level_entry = first_level_entry->table[j];
				if(second_level_entry != NULL) {
					size++;
				}
			}
		}
	}
	return size * 4096;
}
#endif

#ifdef SINGLE_LEVEL_MEMORY
struct Memory* init_memory(FILE* backing_file, uint64_t addr_start, uint64_t addr_end) {
	debug_print("Setting up multi level memoryh!\n");
	struct Memory* mem = malloc(sizeof(struct Memory));
	mem->table = calloc((addr_end-addr_start) / 4096, sizeof(void*));
	mem->addr_start = addr_start;
	mem->addr_end = addr_end;
	if(mem->table == NULL){
		printf("Could not calloc new simulated memory!\n");
		exit(1);
	}
	mem->backing_file = backing_file;
	return mem;
}


int load_page(struct Memory* mem, uint64_t address) {
	//Load from backing file
	fseek(mem->backing_file, (address-mem->addr_start) & ~(0xFFFULL), SEEK_SET);
	if(fread(&(mem->table[(address-mem->addr_start) %4096]), 4096, 1, mem->backing_file) != 1) {
		printf("Unable to read from backing file!%lx\n", (address-mem->addr_start) & ~(0xFFFULL));
		return 1;
	}
}

void free_memory(struct Memory* mem) {
	//TODO free with dfs
}

int read_sim_memory(int amount_memories, struct Memory* memories, uint64_t address, size_t size, void* value) {
	debug_print("Reading sim memory!\n");
	struct Memory* mem = get_memory_for_address(amount_memories, memories, address);
	if(mem == NULL){
		printf("Mem is null!\n");
		return 0;
	}
	if((address & 0xFFF) + size > (1<<12)){
			debug_printf("Reading cross border!\n", 0);
			size_t left_size = (1 << 12 ) - (address & 0xFFF);
			size_t right_size = size - left_size;
	debug_printf("Splitting read, left:%x, right:%x\n", left_size, right_size);
			return read_sim_memory(amount_memories, memories, address, left_size, value)
	+ read_sim_memory(amount_memories, memories, address+left_size, right_size, ((uint8_t*)value)+left_size);
	}
	debug_printf("Looking up memory to read in table:%lx\n", (address-mem->addr_start));
	if(mem->table[(address-mem->addr_start)%4096] == NULL) {
		debug_print("Loading page!\n");
		load_page(mem, address);
	}
	debug_print("Found in table!\n");
	debug_print("Copying memory from memory to output!\n");
	void* memory_loc = ((uint8_t*)mem->table[(address-mem->addr_start)%4096]) + (address & 0xFFF); //TODO handle accesses (check if unaligned can simply be discarded)
	#ifdef USE_MEMCPY
	memcpy(value, memory_loc, size);
	#else
	if(memory_loc != NULL){
		if(size > 16) {
		}else if(size == 8) {
			*(uint64_t*)value = *(uint64_t*)memory_loc;
		}else if(size == 4) {
			*(uint32_t*)value = *(uint32_t*)memory_loc;
		}else if (size == 2) {
			*(uint16_t*)value = *(uint16_t*)memory_loc;
		}else if (size == 1) {
			*(uint8_t*)value = *(uint8_t*)memory_loc;
		}
	}
	#endif
	debug_print("Copyied memory contents to output memory!\n");
	return size;
}



int write_sim_memory(int amount_memories, struct Memory* memories, uint64_t address, size_t size, void* value) {
	debug_print("Writing sim memory!\n");
	struct Memory* mem = get_memory_for_address(amount_memories, memories, address);
	if(mem == NULL) {
		debug_print("Address not in memory!\n");
		return 0;
	}
	if((address & 0xFFF) + size > (1<<12)){
		size_t left_size = (1 << 12 ) - (address & 0xFFF);
		size_t right_size = size - left_size;
		debug_printf("Splitting write: %016lx, left: %d, right: %d!\n", address, left_size, right_size);

		debug_printf("Writing left:\t%p, %lx, %d, %p\n", mem, address, left_size, value);
		debug_printf("Writing right:\t%p, %lx, %d, %p\n", mem, address+left_size, right_size, ((uint8_t*)value)+left_size);
		int written_left = write_sim_memory(amount_memories, memories, address, left_size, value);
		int written_right = write_sim_memory(amount_memories, memories, address+left_size, right_size, ((uint8_t*)value)+left_size);
		return written_left + written_right;
	}


	if(mem->table[(address-mem->addr_start)%4096] == NULL) {
		debug_print("Loading page!\n");
		load_page(mem, address);
	}
	void* memory_loc = (uint8_t*)mem->table[(address-mem->addr_start)%4096]) + (address & 0xFFF);
	#ifdef USE_MEMCPY
	memcpy(memory_loc, value, size);
	#else
	if(memory_loc != NULL){
		if(size > 16) {
			// Intentionally do nothing
		}else if(size == 8) {
			*(uint64_t*)memory_loc = *(uint64_t*)value;
		}else if(size == 4) {
			*(uint32_t*)memory_loc = *(uint32_t*)value;
		}else if (size == 2) {
			*(uint16_t*)memory_loc = *(uint16_t*)value;
		}else if (size == 1) {
			*(uint8_t*)memory_loc = *(uint8_t*)value;
		}
	}
	#endif
	debug_print("Wrote sim memory!\n");
	return size;
}


uint64_t get_size(struct Memory* mem) {
	int amount_pages = (mem->addr_end-mem->addr_start);
	printf("Amount of pages:%d\n", amount_pages);
	uint64_t size = (amount_pages/4096 * sizeof(void*));
	for(int i = 0; i < amount_pages/4096;i++) {
		if(mem->table[i] != NULL) {
			size++;
		}
	}
}

#endif


#ifdef BLOB_MEMORY
struct Memory* init_memory(FILE* backing_file, uint64_t addr_start, uint64_t addr_end) {
	time_t before = time(NULL);
	struct Memory* mem = malloc(sizeof(struct Memory));
	printf("Setting up mem!\n");
	mem->table = malloc(addr_end - addr_start); // Allocate a 1gb slab
	mem->addr_start = addr_start;
	mem->addr_end = addr_end;
	if(mem->table == NULL){
		printf("Could not calloc new simulated memory!\n");
		exit(1);
	}
	mem->backing_file = backing_file;
	for(uint64_t i = 0; i < ((addr_end - addr_start)/1024); i++){
		if(fread(((uint8_t*)mem->table) + (i*1024), 1024, 1, backing_file) != 1) {
			printf("Unable to read from backing file!%p\n", mem->table + (i*1024));
			return NULL;
		}
	}
	printf("Read complete memory from backing file which took: %ld seconds!\n", time(NULL) - before);
	return mem;
}

int write_sim_memory(int amount_memories, struct Memory* memories, uint64_t address, size_t size, void* value) {
	struct Memory* mem = get_memory_for_address(amount_memories, memories, address);
	if(mem == NULL) {
		debug_print("Address not in memory!\n");
		return 0;
	}
	void* memory_loc = ((uint8_t*)mem->table) + address - mem->addr_start;
	#ifdef USE_MEMCPY
	memcpy(memory_loc, value, size);
	#else
	if(memory_loc != NULL){
		if(size > 16) {
			// Intentionally do nothing
		}else if(size == 8) {
			*(uint64_t*)memory_loc = *(uint64_t*)value;
		}else if(size == 4) {
			*(uint32_t*)memory_loc = *(uint32_t*)value;
		}else if (size == 2) {
			*(uint16_t*)memory_loc = *(uint16_t*)value;
		}else if (size == 1) {
			*(uint8_t*)memory_loc = *(uint8_t*)value;
		}
	}
	#endif
	debug_print("Wrote memory!\n");
	return size;
}

__attribute__((always_inline)) inline int read_sim_memory(int amount_memories, struct Memory* memories, uint64_t address, size_t size, void* value) {
	debug_print("Reading memory!\n");
	struct Memory* mem = get_memory_for_address(amount_memories, memories, address);
	debug_printf("Found memory to read%p!\n", mem);
	void* memory_loc = ((uint8_t*)mem->table) + address - mem->addr_start;
	#ifdef USE_MEMCPY
	memcpy(value, memory_loc, size);
	#else
	if(memory_loc != NULL){
		if(size > 16) {
		}else if(size == 8) {
			*(uint64_t*)value = *(uint64_t*)memory_loc;
		}else if(size == 4) {
			*(uint32_t*)value = *(uint32_t*)memory_loc;
		}else if (size == 2) {
			*(uint16_t*)value = *(uint16_t*)memory_loc;
		}else if (size == 1) {
			*(uint8_t*)value = *(uint8_t*)memory_loc;
		}
	}
	#endif
	debug_print("Copies memory to output!\n");
	debug_print("Read memory!\n");
	return size;
}


uint64_t get_size(struct Memory* mem) {
	return mem->addr_end - mem->addr_start;
}
#endif
