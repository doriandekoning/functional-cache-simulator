#include <semaphore.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#include "filereader/sharedmemreader.h"
#include "traceevents.h"


sem_t* setup_sem(char* path, int initalval) {
    sem_t* sem = sem_open(path
    , O_CREAT, S_IRWXU, 1);
    if (sem == NULL) {
        printf("Error opening semaphore!:%d", errno);
        return NULL;
    }
    int sem_val;
    if(sem_getvalue(sem, &sem_val) < 0) {
        printf("Error getting sem value!\n");
        return NULL;
    }
    while(sem_val > initalval) {
        sem_wait(sem);
        sem_val--;
    }
    while(sem_val < initalval) {
        sem_post(sem);
        sem_val++;
    }
    return sem;
}

void* setup_buf(char* path, size_t size) {
    int shared_mem = shm_open(path, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU | S_IRWXG);
    if (shared_mem < 0) {
        printf("In shm_open() of buffer 2");
        return NULL;
    }

    // Resize file
    ftruncate(shared_mem, size);

    void* shared_mem_buffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem, 0);
    if(shared_mem_buffer == NULL) {
        printf("Unable to mmap shared mem!\n");
        return NULL;
    }
    return shared_mem_buffer;
}


struct shared_mem* setup_shared_mem() {
    struct shared_mem* smem = malloc(sizeof(struct shared_mem));
    smem->buf_size = SHARED_MEM_BUF_SIZE;
    smem->write_sem_1 = setup_sem(SEM_WRITE_1_NAME, 1);
    if(smem->write_sem_1 == NULL)return NULL;
    smem->write_sem_2 = setup_sem(SEM_WRITE_2_NAME, 1);
    if(smem->write_sem_2 == NULL)return NULL;
    smem->read_sem_1 = setup_sem(SEM_READ_1_NAME, 0);
    if(smem->read_sem_1 == NULL)return NULL;
    smem->read_sem_2 = setup_sem(SEM_READ_2_NAME, 0);
    if(smem->read_sem_2 == NULL)return NULL;
    smem->current_read_idx = 0;
    smem->current_buf_idx = 1;
    smem->buf_1 = setup_buf(SHARED_MEM_REGION_1_NAME, SHARED_MEM_BUF_SIZE);
    if(smem->buf_1 == NULL)return NULL;
    smem->buf_2 = setup_buf(SHARED_MEM_REGION_2_NAME, SHARED_MEM_BUF_SIZE);
    if(smem->buf_2 == NULL)return NULL;
    printf("Waiting to aquire first read sem!\n");
    sem_wait(smem->read_sem_1);
    printf("Finished smem setup!\n");
    return smem;
}

void read_from_smem(struct shared_mem* smem, size_t size, uint8_t* result) {
     if(smem->current_read_idx + size <= smem->buf_size) {
        if(smem->current_buf_idx == 1) {
            memcpy(result, smem->buf_1 + smem->current_read_idx, size);
        }else{
            memcpy(result, smem->buf_2 + smem->current_read_idx, size);
        }
        smem->current_read_idx+=size;
     }else{
        size_t bytes_first_buf = smem->buf_size - smem->current_read_idx;
        size_t bytes_second_buf = size - bytes_first_buf;
         //First read bytes from first buffer
        if(smem->current_buf_idx == 1) {
            memcpy(result, smem->buf_1 + smem->current_read_idx, bytes_first_buf);
        }else{
            memcpy(result, smem->buf_2 + smem->current_read_idx, bytes_first_buf);
        }
        // Change buffer
        if(smem->current_buf_idx == 1) {
            sem_post(smem->write_sem_1);
            sem_wait(smem->read_sem_2);
            smem->current_buf_idx = 2;
        }else {
            sem_post(smem->write_sem_2);
            sem_wait(smem->read_sem_1);
            smem->current_buf_idx = 1;
        }
        smem->current_read_idx = bytes_second_buf;

        //Copy remainder

        if(smem->current_buf_idx == 1) {
            memcpy(result + bytes_first_buf, smem->buf_1, bytes_second_buf);
        }else{
            memcpy(result + bytes_first_buf, smem->buf_2, bytes_second_buf);
        }
     }
}


uint64_t smem_get_memory_access(struct shared_mem* smem, cache_access* access, bool write) {
    read_from_smem(smem, 1, &(access->cpu));
    read_from_smem(smem, 8, (uint8_t*)&(access->address));
    uint16_t info;
    read_from_smem(smem, 2, (uint8_t*)&info);
    access->type = (info & INFO_STORE_MASK) ? CACHE_EVENT_WRITE : CACHE_EVENT_READ;
    access->size = (1 << (info & INFO_SIZE_SHIFT_MASK));
    access->big_endian = info & INFO_BIG_ENDIAN_MASK;
    if(write) {
        read_from_smem(smem, 8, (uint8_t*)&(access->data));
    }
    return 0;
}

uint64_t smem_get_cr_change(struct shared_mem* smem, cr_change* change) {
    read_from_smem(smem, 1, &(change->cpu));
	read_from_smem(smem, 1, &(change->register_number));
	read_from_smem(smem, 8, (uint8_t*)&(change->new_value));
    return 0;
}

uint64_t smem_get_invlpg(struct shared_mem* smem, uint64_t* addr, uint8_t* cpu) {
    read_from_smem(smem, 8, (uint8_t*)addr);
    read_from_smem(smem, 1, cpu);
    return 0;
}

uint64_t smem_get_tb_start_exec(struct shared_mem* smem, tb_start_exec* tb_start_exec) {
    read_from_smem(smem, 1, &(tb_start_exec->cpu));
    read_from_smem(smem, 8, (uint8_t*)&(tb_start_exec->pc));
    read_from_smem(smem, 2, (uint8_t*)&(tb_start_exec->size));
    return 0;
}


// bool last_was_delta_t = false;
int smem_get_next_event_id(struct shared_mem* smem, uint64_t* delta_t, bool* negative_delta_t, uint8_t* event_id)  {
    uint16_t delta_t_lsb;
    read_from_smem(smem, 2, (uint8_t*)&delta_t_lsb);
    if(delta_t_lsb & (1 << 7)) {
        // if(!last_was_delta_t)printf("Big delta_t%x!\n", delta_t_lsb);

        read_from_smem(smem, 6, (uint8_t*)delta_t);
        *delta_t = (delta_t_lsb & 0x7fff) | (*delta_t << 16);
        *delta_t = __bswap_64(*delta_t) &  ~(0x1ULL << 63 );
        // last_was_delta_t = true;
    }else{
        // if(last_was_delta_t) {
        //     printf("Small delta_t was last!\n");
        //     last_was_delta_t = false;
        // }
        *delta_t = 0x0ULL;
        *delta_t = (__bswap_16(delta_t_lsb) & 0x7fff);
    }
    read_from_smem(smem, 1, event_id);
    *negative_delta_t = (*event_id & (1 << 7)) >> 7;
    *event_id &= 0x7f;
    return 0;
}
