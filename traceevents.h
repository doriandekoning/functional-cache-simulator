#ifndef __TRACEVENTS_H
#define __TRACEVENTS_H


#include <stdint.h>


#define FILE_OPEN_ERROR 1
#define FILE_READ_ERROR 2
#define HEADER_READ_ERROR 3
#define WRONG_HEADER 4

#define MMU_KSMAP_IDX   0
#define MMU_USER_IDX    1
#define MMU_KNOSMAP_IDX 2


#define INFO_SIZE_SHIFT_MASK 0xf /* size shift mask */
#define INFO_SIGN_EXTEND_MASK (1ULL << 4)    /* sign extended (y/n) */
#define INFO_BIG_ENDIAN_MASK (1ULL << 5)    /* big endian (y/n) */
#define INFO_STORE_MASK (1ULL << 6)    /* store (y/n) */
#define INFO_SIGN_EXTEND_MASK (1ULL << 4)


typedef struct access_s {
        uint64_t address;
        uint64_t physaddress;
        uint64_t tick;
        uint8_t cpu;
        uint8_t type;
        uint64_t data;
        uint8_t size;
        uint8_t location;
        bool big_endian;
        uint64_t cr3_val;
} cache_access;

typedef struct tb_start_exec {
    uint64_t pc;
    uint8_t cpu;
    uint16_t size;
} tb_start_exec;


typedef struct cr_change_t {
    uint64_t tick;
    uint8_t cpu;
    uint8_t register_number;
    uint64_t new_value;
} cr_change;


#endif /* __TRACEEVENTS_H */
