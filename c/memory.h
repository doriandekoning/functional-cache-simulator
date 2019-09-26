#ifndef __MEMORY_H
#define __MEMORY_H

#include <stdint.h>
#include <stdlib.h>

struct memory{
	uint64_t* table;
};

struct memory* init_memory();
void free_memory();

int write(struct memory* mem, uint64_t address, size_t size, uint8_t* value);
int read(struct memory* mem, uint64_t address, size_t size, uint8_t* value);

#endif /* __MEMORY_H */
