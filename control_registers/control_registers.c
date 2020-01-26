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


// int map_cpu_id(int cpu_id) {
//     for(int i = 0; i < AMOUNT_SIMULATED_PROCESSORS; i++) {
//         if(cpu_id_map[i] == cpu_id){
//             return i;
//         } else if(cpu_id_map[i] == INT_MAX) {
//             cpu_id_map[i] = cpu_id;
//             return i;
//         }
//     }
//     printf("Unable to map cpu:%lx\n", cpu_id);
//     exit(0);
// }

int read_cr_values_from_dump(char* cr_values_path, ControlRegisterValues control_register_values) {
	debug_printf("Opening CR Values file at:%s\n", cr_values_path);
	FILE* f = fopen(cr_values_path, "rb");
	if(f == NULL){
		printf("Unable to open control register initial values file!\n");
		return 1;
	}
	uint64_t processor = 0;
  	int string_offset = 0;
	uint64_t value;
	for(int i = 0; i < AMOUNT_SIMULATED_PROCESSORS; i++){
   		fscanf(f, "%llx\n", &processor);
    	for(int j = 0; j<5; j++) {

      		fscanf(f, "%llx\n", &value);
			set_cr_value(control_register_values, processor, j, value);
	    }
		// debug_printf("Paging is: %s\n", paging_enabled(control_register_values, tmp_access->cpu) ? "enabled" : "disabled");
		debug_printf("Initial CR values for processor:%lx\n", processor);
		for(int i = 0; i < 5; i++ ) {
			printf("Processor[%lx]CR%d=0x%lx\n", processor, i, get_cr_value(control_register_values, processor, i));
		}
	}
	return 0;
}
