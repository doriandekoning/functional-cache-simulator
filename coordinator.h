#ifndef COORDINATOR_H
#define COORDINATOR_H

#include <mpi.h>
//#include "pagetable.h"
#include <stdint.h>
#include <stdbool.h>


int run_coordinator(int world_size, char* input_pagetables, bool input_is_pipe, char* cr_values_path);
uint64_t get_physical_address(uint64_t address, uint64_t cr3, int cpu);

#endif /* COORDINATOR_H */
