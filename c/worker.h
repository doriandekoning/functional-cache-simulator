#ifndef WORKER_H
#define WORKER_H
#include "pagetable.h"

typedef struct cr3_list_t {
    uint64_t cr3_value;
    pagetable * pagetable;
    cr3_list* next;
} cr3_list;

int run_worker(int world_size);
uint64_t get_physical_address(uint64_t address, uint64_t cr3, int cpu);

#endif /* WORKER_H */
