
#ifndef FILEREADER_H
#define FILEREADER_H
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "cache/state.h"
#include "traceevents.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>



int read_header(FILE* file);
uint64_t get_memory_access(FILE* file, cache_access* access, bool write);
uint64_t get_cr_change(FILE* file, cr_change* change);
uint64_t get_invlpg(FILE* file, uint64_t* addr, uint8_t* cpu);
uint64_t get_tb_start_exec(FILE* file, tb_start_exec* tb_start_exec);
int get_next_event_id(FILE* file, uint64_t* delta_t, bool* negative_delta_t, uint8_t* event_id) ;

#endif //FILEREADER_H
