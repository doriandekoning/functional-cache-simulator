#ifndef __CONTROL_REGISTERS
#define __CONTROL_REGISTERS

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define AMOUNT_CONTROL_REGISTERS 8

#define CR0_PE_MASK (1 << 0) // Protected mode enabled (also paging)
#define CR0_PG_MASK (1 << 31)
#define PE_MASK (1 << 0)
#define PG_MASK (1 << 31)

typedef uint64_t* ControlRegisterValues;

ControlRegisterValues init_control_register_values(uint8_t amount_cpus);

uint64_t get_cr_value(ControlRegisterValues values, uint8_t cpu, int register_number);

void set_cr_value(ControlRegisterValues values, uint8_t cpu, int register_number, uint64_t new_value);

bool paging_enabled(ControlRegisterValues values, uint8_t cpu);

int read_cr_values_from_dump(char* cr_values_path, ControlRegisterValues control_register_values, int amount_cpus);


#endif /* __CONTROL_REGISTERS*/
