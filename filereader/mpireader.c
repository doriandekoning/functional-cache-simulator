#ifdef USING_MPI
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>


#include "filereader/sharedmemreader.h"
#include "filereader/mpireader.h"
#include "traceevents.h"


struct mpi_buffer* setup_mpi_buffer(size_t size, int sender_rank) {
    struct mpi_buffer* ret = malloc(sizeof(struct mpi_buffer));
    ret->current_read_idx = 0;
    ret->buf_size = size;
    ret->current_buf_idx = -1;
    ret->buf_0 = malloc(SHARED_MEM_BUF_SIZE);
    printf("malloced buf!%p\n", ret->buf_0);
    ret->buf_1 = malloc(SHARED_MEM_BUF_SIZE);
    ret->sender_rank = sender_rank;

    if(MPI_Irecv(ret->buf_0, SHARED_MEM_BUF_SIZE, MPI_CHAR, ret->sender_rank, 1, MPI_COMM_WORLD, &ret->open_request) != MPI_SUCCESS ) {
        printf("Unable to receive!\n");
        return NULL; //TODO check
    }
    return ret;
}


void read_from_buffer(struct mpi_buffer* buf, size_t size, uint8_t* result) {
    if(buf->current_buf_idx < 0){
        if(MPI_Wait(&buf->open_request, MPI_STATUS_IGNORE)) {
            printf("Unable to wait for mpi!\n");
            return;
        }
        if(MPI_Irecv(buf->buf_1, SHARED_MEM_BUF_SIZE, MPI_CHAR, buf->sender_rank, 1, MPI_COMM_WORLD, &buf->open_request) != MPI_SUCCESS ) {
            printf("Unable to receive!\n");
            return; //TODO check
        }
        printf("Received first buffer!\n");
        buf->current_buf_idx = 0;
    }
     if(buf->current_read_idx + size <= buf->buf_size) {
        if(buf->current_buf_idx == 0) {
            memcpy(result, buf->buf_0 + buf->current_read_idx, size);
        }else{
            memcpy(result, buf->buf_1 + buf->current_read_idx, size);
        }
        buf->current_read_idx+=size;
     }else{
        size_t bytes_first_buf = buf->buf_size - buf->current_read_idx;
        size_t bytes_second_buf = size - bytes_first_buf;
         //First read bytes from first buffer
        if(buf->current_buf_idx == 0) {
            memcpy(result, buf->buf_0 + buf->current_read_idx, bytes_first_buf);
        }else{
            memcpy(result, buf->buf_1 + buf->current_read_idx, bytes_first_buf);
        }
        // Change buffer
        uint8_t* buff_to_use;
        if(buf->current_buf_idx == 0) {
            buff_to_use = buf->buf_0;
        }else{
            buff_to_use = buf->buf_1;
        }
        if(MPI_Wait(&buf->open_request, MPI_STATUS_IGNORE)) {
            printf("Unable to wait for mpi!\n");
            return;
        }
        if(MPI_Irecv(buff_to_use, SHARED_MEM_BUF_SIZE, MPI_CHAR, buf->sender_rank, 1, MPI_COMM_WORLD, &buf->open_request) != MPI_SUCCESS ) {
            printf("Unable to receive!\n");
            return;
        }
        if(buf->current_buf_idx == 0) {
            buf->current_buf_idx = 1;
        }else{
            buf->current_buf_idx = 0;

        }
        buf->current_read_idx = bytes_second_buf;

        //Copy remainder

        if(buf->current_buf_idx == 0) {
            memcpy(result + bytes_first_buf, buf->buf_0, bytes_second_buf);
        }else{
            memcpy(result + bytes_first_buf, buf->buf_1, bytes_second_buf);
        }
     }
}


uint64_t mpi_buffer_get_memory_access(struct mpi_buffer* buf, cache_access* access, bool write) {
    read_from_buffer(buf, 1, &(access->cpu));
    read_from_buffer(buf, 8, (uint8_t*)&(access->address));
    uint16_t info;
    read_from_buffer(buf, 1, (uint8_t*)&info);
    access->type = (info & INFO_STORE_MASK) ? CACHE_EVENT_WRITE : CACHE_EVENT_READ;
    access->size = (1 << (info & INFO_SIZE_SHIFT_MASK));
    if(write) {
        read_from_buffer(buf, 8, (uint8_t*)&(access->data));
    }
    return 0;
}

uint64_t mpi_buffer_get_cr_change(struct mpi_buffer* buf, cr_change* change) {
    read_from_buffer(buf, 1, &(change->cpu));
	read_from_buffer(buf, 1, &(change->register_number));
	read_from_buffer(buf, 8, (uint8_t*)&(change->new_value));
    return 0;
}

uint64_t mpi_buffer_get_invlpg(struct mpi_buffer* buf, uint64_t* addr, uint8_t* cpu) {
    read_from_buffer(buf, 8, (uint8_t*)addr);
    read_from_buffer(buf, 1, cpu);
    return 0;
}

uint64_t mpi_buffer_get_tb_start_exec(struct mpi_buffer* buf, tb_start_exec* tb_start_exec) {
    read_from_buffer(buf, 1, &(tb_start_exec->cpu));
    read_from_buffer(buf, 8, (uint8_t*)&(tb_start_exec->pc));
    read_from_buffer(buf, 2, (uint8_t*)&(tb_start_exec->size));
    return 0;
}


// bool last_was_delta_t = false;
int mpi_buffer_get_next_event_id(struct mpi_buffer* buf, uint64_t* delta_t, bool* negative_delta_t, uint8_t* event_id)  {
    uint16_t delta_t_lsb;
    read_from_buffer(buf, 2, (uint8_t*)&delta_t_lsb);
    if(delta_t_lsb & (1 << 7)) {
        read_from_buffer(buf, 6, (uint8_t*)delta_t);
        *delta_t = (delta_t_lsb & 0x7fff) | (*delta_t << 16);
        *delta_t = __bswap_64(*delta_t) &  ~(0x1ULL << 63 );
    }else{
        *delta_t = 0x0ULL;
        *delta_t = (__bswap_16(delta_t_lsb) & 0x7fff);
    }
    read_from_buffer(buf, 1, event_id);
    *negative_delta_t = (*event_id & (1 << 7)) >> 7;
    *event_id &= 0x7f;
    return 0;
}


#endif /*USING_MPI*/
