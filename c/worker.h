#ifndef WORKER_H
#define WORKER_H

int run_worker(int world_size);
uint64_t get_physical_address(uint64_t address, uint64_t cr3, int cpu);

#endif /* WORKER_H */
