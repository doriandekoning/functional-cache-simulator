#ifndef __PAGETABLE_H
#define __PAGETABLE_H

#include <stdint.h>
#include <stdbool.h>

#include "memory/memory.h"


#define NOTFOUND_ADDRESS        (1UL << 52) // Some non cannocial address which falls into the unused hole 0000800000000000-ffff7fffffffffff

uint64_t vaddr_to_phys(struct memory* mem, uint64_t l1_phys, uint64_t virtaddress, bool debug);
int print_pagetable(struct memory* mem, uint64_t cr3);
int read_pagetable(struct memory* mem, char* path);
#endif
