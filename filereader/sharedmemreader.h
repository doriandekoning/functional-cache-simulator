
#ifndef FILEREADER_H
#define FILEREADER_H
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "cache/state.h"
#include "traceevents.h"

#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>
#include <stdio.h>

// These values are hardcoded in QEMU so we can also just hardcode them here
#define SEM_WRITE_1_NAME "/qemu_st_sem_write_1"
#define SEM_WRITE_2_NAME "/qemu_st_sem_write_2"
#define SEM_READ_1_NAME "/qemu_st_sem_read_1"
#define SEM_READ_2_NAME "/qemu_st_sem_read_2"
#define SHARED_MEM_REGION_1_NAME "/qemu_simple_trace_buffer_1"
#define SHARED_MEM_REGION_2_NAME "/qemu_simple_trace_buffer_2"
#define SEM_STARTUP_NAME "/memtracing_start_sem"
#define SHARED_MEM_BUF_SIZE 1024*1024*8


struct shared_mem {
    uint8_t* buf_1, *buf_2;
    sem_t *write_sem_1, *write_sem_2, *read_sem_1, *read_sem_2, *start_sem;
    size_t current_read_idx;
    uint8_t current_buf_idx;
    size_t buf_size;
};



struct shared_mem* setup_shared_mem();
uint64_t smem_get_memory_access(struct shared_mem* smem, cache_access* access, bool write);
uint64_t smem_get_cr_change(struct shared_mem* smem, cr_change* change);
uint64_t smem_get_invlpg(struct shared_mem* smem, uint64_t* addr, uint8_t* cpu);
uint64_t smem_get_tb_start_exec(struct shared_mem* smem, tb_start_exec* tb_start_exec);
int smem_get_next_event_id(struct shared_mem* smem, uint64_t* delta_t, bool* negative_delta_t, uint8_t* event_id) ;

#endif //FILEREADER_H

