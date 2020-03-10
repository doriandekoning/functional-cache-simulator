#ifndef COORDINATOR_H
#define COORDINATOR_H

#include <mpi.h>
//#include "pagetable.h"
#include <stdint.h>
#include <stdbool.h>


int run_coordinator(int world_size, int amount_cpus, char* memory_range_base_path);
uint64_t get_physical_address(uint64_t address, uint64_t cr3);

#endif /* COORDINATOR_H */
