#ifndef __MEMORY_H
#define __MEMORY_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

struct Memory{
	struct Memory** table;
	FILE* backing_file;
};

struct Memory* init_memory(FILE* backing_file);
void free_memory();


void convert_to_little_endian(size_t size, void* value);
int write_sim_memory(struct Memory* mem, uint64_t address, size_t size, void* value);
int read_sim_memory(struct Memory* mem, uint64_t address, size_t size, void* value);

#endif /* __MEMORY_H */
