#include <stdint.h>
#include <stdio.h>

#include "control_registers/control_registers.h"
#include "config.h"

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
    uint64_t cr0_value = get_cr_value(values, cpu, 0);
    return (cr0_value & CR0_PE_MASK) && (cr0_value & CR0_PG_MASK);
}


int read_cr_values_from_dump(char* cr_values_path, ControlRegisterValues control_register_values, int amount_cpus) {
	printf("Opening CR Values file at:%s\n", cr_values_path);
	FILE* f = fopen(cr_values_path, "rb");
	if(f == NULL){
		printf("Unable to open control register initial values file!\n");
		return 1;
	}
	uint64_t processor = 0;
	uint64_t value;
	for(int i = 0; i < amount_cpus; i++){
   		fscanf(f, "%lu\n", &processor);
    	for(int j = 0; j<5; j++) {
      		fscanf(f, "%lx\n", &value);
			set_cr_value(control_register_values, processor, j, value);
			debug_printf("CPU: %d Read CR[%d]:%lx\n", i, j, value);
	    }
		// debug_printf("Paging is: %s\n", paging_enabled(control_register_values, tmp_access->cpu) ? "enabled" : "disabled");
		debug_printf("Initial CR values for processor:%lx\n", processor);
	}
	return 0;
}
