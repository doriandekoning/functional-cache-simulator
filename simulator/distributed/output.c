#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <mpi.h>

#include "simulator/distributed/output.h"
#include "simulator/distributed/mpi_datatype.h"
#include "config.h"


#define DEBUG 1

struct WorkerBuffer {
    struct CacheMiss misses[1024];
    struct WorkerBuffer *next;
    uint32_t buffer_offset;
};

struct RegisteredWorker {
    struct WorkerBuffer *first_buf;
};


int receive(MPI_Datatype mpi_cache_miss_dt, struct RegisteredWorker* workers, int amount_workers) {
    struct WorkerBuffer *cur_buffer;
    MPI_Status status;
    debug_printf("[OUTPUT]Receiving%d\n", sizeof(struct WorkerBuffer));
    struct WorkerBuffer* new_buf = malloc(sizeof(struct WorkerBuffer));
    new_buf->buffer_offset = 0;
    if(MPI_Recv(new_buf->misses, 1024, mpi_cache_miss_dt, MPI_ANY_SOURCE, MPI_TAG_CACHE_MISS, MPI_COMM_WORLD, &status) != MPI_SUCCESS) {
        printf("Unable to receive cache misses!");
        return 1;
    }

    // Find worker from which we received by rank
    cur_buffer = workers[status.MPI_SOURCE].first_buf;
    if(workers[status.MPI_SOURCE].first_buf== NULL) {
        workers[status.MPI_SOURCE].first_buf = new_buf;
    }else{
        // If worker already has one or multiple buffers find the last
        while(cur_buffer->next != NULL) {
            cur_buffer = cur_buffer->next;
        }
        // Update last buffer
        cur_buffer->next = new_buf;
    }
    debug_print("[OUTPUT]Received!\n");
    return status.MPI_SOURCE;
}

int initialize(MPI_Datatype mpi_cache_miss_dt, struct RegisteredWorker* workers, int amount_workers) {
    bool has_received_for_worker[amount_workers];
    for(int i = 0; i < amount_workers; i++)  {
        has_received_for_worker[i] = false;
    }
    while(1) {
        printf("[OUTPUT]Receiving\n");
        has_received_for_worker[receive(mpi_cache_miss_dt, workers, amount_workers)] = true;
        for(int i = 0; i < amount_workers; i++) {
            if(!has_received_for_worker ){
                continue;
            }
        }
        break;
    }
}

int run_output(int world_size, int world_rank, int amount_workers, char* output_file_path) {
    printf("[OUTPUT]started!\n");
    MPI_Datatype mpi_cache_miss_dt;
    struct RegisteredWorker* workers;
    FILE* out;




    if(get_mpi_cache_miss_datatype(&mpi_cache_miss_dt)){
        printf("Unable to setup datatype\n");
        return 1;
    }
    printf("[OUTPUT]Writing to file:%s\n", output_file_path);
    out = fopen(output_file_path, "wb");
    if(out == NULL) {
        printf("Unable to open output file!\n");
        return 1;
    }

    workers = calloc(amount_workers, sizeof(struct RegisteredWorker));
    int smallest_idx = 0;
    uint64_t smallest_t = 0;
    uint64_t cur_t;
    struct CacheMiss *cur_miss;
    printf("[OUTPUT]initializing\n");
    initialize(mpi_cache_miss_dt, workers, amount_workers);
    printf("[OUTPUT]Initialized\n");

    //TODO Recv untill all workers have at least one buffer
    while(1) {
        smallest_idx = 0;
        smallest_t = workers[0].first_buf->misses[workers[0].first_buf->buffer_offset].timestamp;
        for(int i = 1; i < amount_workers; i++ ){
            cur_t = workers[i].first_buf->misses[workers[i].first_buf->buffer_offset].timestamp;
            if(cur_t < smallest_t) {
                smallest_idx = i;
                smallest_t = cur_t;
            }
        }

        cur_miss = &(workers[smallest_idx].first_buf->misses[workers[smallest_idx].first_buf->buffer_offset]);
        if(fwrite(&cur_miss, sizeof(cur_miss), 1, out) != 1) {
            printf("Unable to write output!\n");
            return 1;
        }
        //TODO change to write to file
        workers[smallest_idx].first_buf->buffer_offset++;
        // Check if buffer is empty
        if(workers[smallest_idx].first_buf->buffer_offset == 1024){
            struct WorkerBuffer* old_buf = workers[smallest_idx].first_buf;
            workers[smallest_idx].first_buf = workers[smallest_idx].first_buf->next;
            free(old_buf);
            // If there is no next buffer recv a new one
            if(workers[smallest_idx].first_buf == NULL ){
                int sender = -1;
                do {
                    sender = receive(mpi_cache_miss_dt, workers, amount_workers);
                }while(sender != smallest_idx);
            }
        }
    }
}


#undef DEBUG
