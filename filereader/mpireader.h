
#ifdef USING_MPI
#ifndef _BUFFERREADER_H
#define _BUFFERREADER_H
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "cache/state.h"
#include "traceevents.h"

#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>
#include <stdio.h>

struct mpi_buffer {
    uint8_t* buf_0, *buf_1;
    size_t current_read_idx;
    size_t buf_size;
    int current_buf_idx;
    int sender_rank;
    MPI_Request open_request;
};



struct mpi_buffer* setup_mpi_buffer(size_t size, int sender_rank);
uint64_t mpi_buffer_get_memory_access(struct mpi_buffer* buf, cache_access* access, bool write);
uint64_t mpi_buffer_get_cr_change(struct mpi_buffer* buf, cr_change* change);
uint64_t mpi_buffer_get_invlpg(struct mpi_buffer* buf, uint64_t* addr, uint8_t* cpu);
uint64_t mpi_buffer_get_tb_start_exec(struct mpi_buffer* buf, tb_start_exec* tb_start_exec);
int mpi_buffer_get_next_event_id(struct mpi_buffer* buf, uint64_t* delta_t, bool* negative_delta_t, uint8_t* event_id);

#endif //_BUFFERREADER_H

#endif /*USING_MPI*/
