#ifndef __MEMORY_H
#define __MEMORY_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;


struct Memory{
	struct Memory** table;
	uint64_t addr_start, addr_end;
	FILE* backing_file;
};



struct Memory* init_memory(FILE* backing_file, uint64_t addr_start, uint64_t addr_end);
void free_memory();


void convert_to_little_endian(size_t size, void* value);
void* get_memory_pointer(int amount_memories, struct Memory* memories, uint64_t address);
int write_sim_memory(int amount_memories, struct Memory* memories, uint64_t address, size_t size, void* value);
int read_sim_memory(int amount_memories, struct Memory* memories, uint64_t address, size_t size, void* value);
uint64_t get_size(struct Memory* mem);

#endif /* __MEMORY_H */
