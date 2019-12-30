
#ifndef PIPEREADER_H
#define PIPEREADER_H
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "cache/state.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


#define PIPE_OPEN_ERROR 1
#define PIPE_READ_ERROR 2
#define HEADER_READ_ERROR 3
#define WRONG_HEADER 4

#define MMU_KSMAP_IDX   0
#define MMU_USER_IDX    1
#define MMU_KNOSMAP_IDX 2

//TODO refactor away from bitfields
struct qemu_mem_info {
     uint8_t size_shift : 2; /* interpreted as "1 << size_shift" bytes */
     bool    sign_extend: 1; /* sign-extended */
     uint8_t endianness : 1; /* 0: little, 1: big */
     bool    store      : 1; /* wheter it's a store operation */
};


typedef struct access_s {
        uint64_t address;
        uint64_t tick;
        uint8_t cpu;
        uint8_t type;
        uint64_t data;
        uint8_t size;
        bool user_access;
        uint8_t location;
        bool big_endian;
} cache_access;

typedef struct cr_change_t {
    uint64_t tick;
    uint64_t cpu;
    uint8_t register_number;
    uint64_t new_value;
} cr_change;

int read_header(FILE* pipe);
uint64_t get_memory_access(FILE* pipe, cache_access* access, bool write);
uint64_t get_cr_change(FILE* pipe, cr_change* change);
int get_next_event_id(FILE* pipe, uint64_t* delta_t, bool* negative_delta_t, uint8_t* event_id) ;


#endif //PIPEREADER_H
