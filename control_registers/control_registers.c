#include <stdint.h>
#include <control_registers/control_registers.h>

ControlRegisterValues init_control_register_values(uint8_t amount_cpus) {
    return calloc(sizeof(uint64_t), amount_cpus * AMOUNT_CONTROL_REGISTERS);
}

uint64_t get_cr_value(ControlRegisterValues values, uint8_t cpu, int register_number) {
    return values[(cpu*AMOUNT_CONTROL_REGISTERS) + register_number];
}

void set_cr_value(ControlRegisterValues values, uint8_t cpu, int register_number, uint64_t new_value) {
    values[(cpu*AMOUNT_CONTROL_REGISTERS) + register_number] = new_value;
}

bool paging_enabled(ControlRegisterValues values, uint8_t cpu) {
    return (values[cpu*AMOUNT_CONTROL_REGISTERS] & PE_MASK) && (values[cpu  * AMOUNT_CONTROL_REGISTERS]& PG_MASK);
}
