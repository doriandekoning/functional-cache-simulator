
#ifndef PIPEREADER_H
#define PIPEREADER_H

#define PIPE_OPEN_ERROR 1
#define PIPE_READ_ERROR 2
#define HEADER_READ_ERROR 3
#define WRONG_HEADER 4


struct qemu_mem_info {
     uint8_t size_shift : 2; /* interpreted as "1 << size_shift" bytes */
     bool    sign_extend: 1; /* sign-extended */
     uint8_t endianness : 1; /* 0: little, 1: big */
     bool    store      : 1; /* wheter it's a store operation */
};

typedef struct cr3_change_t {
    uint64_t tick;
    uint64_t cpu;
    uint64_t new_cr3;
} cr3_change;

int read_header(FILE * pipe);
int get_cache_access(FILE* pipe, cache_access* access);
int get_cr3_change(FILE* pipe, cr3_change* update);
int get_next_event_id(FILE* pipe);


#endif //PIPEREADER_H
