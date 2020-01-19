#ifndef MAPPINGREADER_H
#define MAPPINGREADER_H

#include <stdint.h>

struct EventIDMapping {
    uint8_t guest_update_cr;
    uint8_t instruction_fetch;
    uint8_t guest_mem_load_before_exec;
    uint8_t guest_mem_store_before_exec;
    uint8_t guest_flush_tlb_invlpg;
    uint8_t guest_start_exec_tb;
};

int read_mapping(char* location, struct EventIDMapping *);
void print_mapping(struct EventIDMapping* mapping);

#endif // MAPPINGREADER_H
