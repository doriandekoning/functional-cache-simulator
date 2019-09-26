#ifndef COORDINATOR_H
#define COORDINATOR_H

#include <mpi.h>
#include "pagetable.h"


struct pagetable_list {
    uint64_t cr3_value;
    pagetable* pagetable;
    struct pagetable_list* next;
};

int run_coordinator(int world_size, char* input_pagetables, char* input_file);
uint64_t get_physical_address(uint64_t address, uint64_t cr3, int cpu);
pagetable* find_pagetable_for_cr3(uint64_t cr3);

#endif /* COORDINATOR_H */
