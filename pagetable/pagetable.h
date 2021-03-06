#ifndef __PAGETABLE_H
#define __PAGETABLE_H

#include <stdint.h>
#include <stdbool.h>

#include "memory/memory.h"

#define NOTFOUND_ADDRESS        (1UL << 52) // Some non cannocial address which falls into the unused hole 0000800000000000-ffff7fffffffffff

uint32_t vaddr_to_phys32(int amount_mems, struct Memory* mems, uint32_t cr3_value, uint32_t virtaddress, bool debug);
uint64_t vaddr_to_phys(int amount_mems, struct Memory* mems, uint64_t l1_phys, uint64_t virtaddress, bool* is_user_page, bool* cache_disable);
int print_pagetable(int amount_mems, struct Memory* mems, uint64_t cr3);
int read_pagetable(int amount_mems, struct Memory* mems, char* path);
#endif
