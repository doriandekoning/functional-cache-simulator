
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

void* read_header(FILE * pipe);
cache_access* get_next_access(FILE* pipe, cache_access* access);

#endif //PIPEREADER_H
