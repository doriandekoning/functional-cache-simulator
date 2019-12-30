#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef SIMULATE_ADDRESS_TRANSLATION
#ifndef SIMULATE_MEMORY
#error "Not able to simulate address translation without simulating memory"
#endif
#endif

//TODO move each type of reader: printtrace, simplecachesim, simulator to a separate directory
#include "config.h"
#include "pipereader.h"
#include "cache/state.h"
#include "mappingreader/mappingreader.h"
#include "control_registers/control_registers.h"

#ifdef SIMULATE_MEMORY
#include "memory/memory.h"
#endif

#ifdef SIMULATE_ADDRESS_TRANSLATION
#include "pagetable/pagetable.h"
#endif

#define PG_MASK (1 << 31)
#define PE_MASK (1 << 0)


int main(int argc, char **argv) {
	struct EventIDMapping trace_mapping;
	FILE* in;
	unsigned char buf[4048];

	if(argc < 3) {
		printf("Atleast two arguments must be provided, first the mapping path, second the trace file\n");
		return 1;
	}

	if(read_mapping(argv[1], &trace_mapping)){
		printf("Could not read trace mapping!\n");
		return 1;
	}
	debug_printf("Event id mapping:\n%d->%s\n%d->%s\n%d->%s\n",
		trace_mapping.guest_update_cr, "guest_update_cr",
		trace_mapping.guest_mem_load_before_exec, "guest_mem_load_before_exec",
		trace_mapping.guest_mem_store_before_exec, "guest_mem_store_before_exec"
	);
	debug_printf("Using: \"%s\" as input\n",  argv[2]);
	in = fopen(argv[2], "r");
	if(in == NULL) {
		printf("Could not open file!\n");
		return 1;
	}

	if(read_header(in) != 0) {
		printf("Unable to read header\n");
		return 1;
	}
	CacheState cache_state = calloc(CACHE_AMOUNT_LINES, sizeof(struct CacheEntry*));
	if(cache_state == NULL) {
		printf("Unable to allocate cache state!\n");
		return 1;
	}
	uint64_t cr_update_count = 0;
	uint64_t amount_reads = 0;
	uint64_t amount_writes = 0;
	uint64_t amount_accesses = 0;
	uint64_t amount_miss = 0;
	uint64_t largest_delta_time = 0;
	uint64_t unable_to_translate = 0;
	uint64_t amount_big_endian = 0;
	uint64_t amount_little_endian = 0;
	uint64_t translated = 0;
	uint64_t amount_user_accesses = 0;
	uint64_t physical_access = 0;
	uint64_t amount_zero_writes = 0;
	ControlRegisterValues control_register_values = init_control_register_values(4); //TODO configure
	cache_access* tmp_access = malloc(sizeof(cache_access));
	cr_change* tmp_cr_change = malloc(sizeof(cr_change));
	uint64_t delta_t = 0;
	uint64_t current_timestamp = 0;
	#ifdef SIMULATE_MEMORY
	struct memory* simulated_memory = init_memory();
	#endif
	uint8_t next_event_id;
	bool negative_delta_t, hit;
	bool paging_is_enabled;
	uint64_t physical_address;
	bool failed = false;
	while(true) {

		if(get_next_event_id(in, &delta_t, &negative_delta_t, &next_event_id) == -1) {
			printf("Unable to read ID of next event!\n");
			break;
		}
		if(!negative_delta_t) {
			current_timestamp += delta_t;
		}else{
			current_timestamp -= delta_t;
		}
		if(next_event_id == trace_mapping.guest_mem_load_before_exec || next_event_id == trace_mapping.guest_mem_store_before_exec ) {
			if(get_memory_access(in, tmp_access, next_event_id == trace_mapping.guest_mem_store_before_exec)) {
				printf("Can not read cache access\n");
				break;
			}
			tmp_access->tick = current_timestamp;
			if(next_event_id == trace_mapping.guest_mem_load_before_exec) {
				amount_reads++;
			}else{
				amount_writes++;
			}
			amount_accesses++;
			#ifdef SIMULATE_ADDRESS_TRANSLATION
			if(paging_enabled(control_register_values, tmp_access->cpu)) {
				physical_address = vaddr_to_phys(simulated_memory, get_cr_value(control_register_values, tmp_access->cpu, 3) & 0x3fffffffff000ULL, tmp_access->address, false);
				if(physical_address == NOTFOUND_ADDRESS) {
					unable_to_translate++;
					printf("AT TICK:%lu\n", tmp_access->tick);
					printf("Unable to translate \t%lx CR0:[%lx] CR3:[%lx] CR4:[%lx] and cpu %d of memory access at location: %d, %lx\n", tmp_access->address, get_cr_value(control_register_values, tmp_access->cpu, 0), get_cr_value(control_register_values, tmp_access->cpu, 3),get_cr_value(control_register_values, tmp_access->cpu, 4), tmp_access->cpu, tmp_access->location, tmp_access->tick);
					uint64_t cr3 = get_cr_value(control_register_values, tmp_access->cpu, 3);
					vaddr_to_phys(simulated_memory, cr3 & 0x3fffffffff000ULL, tmp_access->address, true);
					printf("CR3 used:%lx, %lx\n", cr3, cr3 & 0x3fffffffff000ULL);
					uint64_t val;
					if(read_sim_memory(simulated_memory, get_cr_value(control_register_values, tmp_access->cpu, 3) + (8*(tmp_access->address & 0xfff)), 8, &val) != 8) {
						printf("PANIEK!\n");
					}
					printf("Value at cr3 location: %lx\n", val);
					break;
				}else{
					translated++;
				}
				debug_printf("Virtual address: %lx translated to physical address: %lx\n", tmp_access->address, physical_address);
			}else{
				physical_access++;
				physical_address = tmp_access->address;
			}
			#endif

			bool write = (tmp_access->type == CACHE_WRITE);
			if(write && tmp_access->data == 0) {
				amount_zero_writes++;
			}
			if(tmp_access->user_access) {
				amount_user_accesses++;
			}

			// if(amount_accesses > (98923991-100000) && write) {
			// 	printf("Write: to address \t%lx(%lx) with data %lx, size: %d, %lx, CR3[%lx]\n", tmp_access->address, physical_address, tmp_access->data, tmp_access->size, tmp_access->tick, get_cr_value(control_register_values, tmp_access->cpu, 3));
			// }
			// hit = perform_cache_access(cache_state, 0, physical_address, write);
			// if(!hit) {
			// 	amount_miss++;
			// }
			#ifdef SIMULATE_MEMORY
			//Assume write through (all write  access are directly written to memory)
			if(write ) { //&& !tmp_access->user_access
				// printf("Writing %lx -- at %lx\n", tmp_access->data, physical_address);

				if(physical_address >= 0x2a12ff8 && physical_address <= (0x2a12ff8+8) ) {
					printf("Write to %lx(%lx), %lx, size: %d, current CR3: %lx\n", tmp_access->address, physical_address, tmp_access->data, tmp_access->size, get_cr_value(control_register_values, tmp_access->cpu, 3));
				}
				// convert_to_little_endian(size, value);
				if(tmp_access->size > 1 && tmp_access->big_endian) {
					amount_big_endian++;
				}else if(tmp_access->size > 1){
					amount_little_endian++;
				}

				// uint64_t value_before, value_after;
				// read_sim_memory(simulated_memory, 0x2a12000, 8, &value_before);
				uint64_t value_before;
				if(physical_address >= 0x2a12ff8 && physical_address <= (0x2a12ff8+8) ) {
					read_sim_memory(simulated_memory, 0x2a12ff8,  8, &value_before);
				}


				if(write_sim_memory(simulated_memory, physical_address,  tmp_access->size, &(tmp_access->data)) !=  tmp_access->size) {
			 		printf("Unable to write memory!\n");
					return 1;
			 	}

				if(physical_address >= 0x2a12ff8 && physical_address <= (0x2a12ff8+8) ) {
					uint64_t value_after;
					read_sim_memory(simulated_memory, 0x2a12ff8,  8, &value_after);
					printf("Value before: %lx, after: %lx\n", value_before, value_after);
				}


			}

			#endif /* SIMULATE_MEMORY */

		} else if(next_event_id == trace_mapping.guest_update_cr) {
			cr_update_count++;

			if(get_cr_change(in, tmp_cr_change)) {
				printf("Can not read cr change\n");
				break;
			}
			tmp_cr_change->tick = current_timestamp;

			set_cr_value(control_register_values, tmp_cr_change->cpu, tmp_cr_change->register_number, tmp_cr_change->new_value);
			debug_printf("CR[%d] updated to: %lx\n", tmp_cr_change->register_number, tmp_cr_change->new_value);
			bool paging_was_enabled = paging_is_enabled;
			paging_is_enabled = paging_enabled(control_register_values, tmp_access->cpu);
			if(paging_was_enabled != paging_is_enabled) {
				if(paging_is_enabled) {
					printf("Enabled paging!\n");
				}else {
					printf("Disabled paging!\n");
				}
			}
		}else {
			printf("Unknown eventid: %x\n", next_event_id);
			break;
		}
		if(amount_accesses % 10000000 == 0) {
			printf("%lu million accesses processed!\n", amount_accesses / 1000000);
		}
		// if(amount_accesses == 1500000) {
		// 	printf("FInished!\n");
		// 	break;
		// }


	}
	printf("Amount of CR updates:\t%lu\n", cr_update_count);
	printf("Reads:\t\t%lu\n", amount_reads);
	printf("Writes:\t\t%lu\n", amount_writes);
	printf("Amount accesses:\t\t\t%lu\n", amount_accesses);
	printf("Amount access miss:\t\t\t%lu\n", amount_miss);
	printf("Amount access hit:\t\t\t%lu\n", amount_accesses-amount_miss);
	printf("Amount access translated:\t\t%lu\n", translated);
	printf("Amount physical accesses:\t\t%lu\n", physical_access);
	printf("Amount access unable to translate:\t%lu\n", unable_to_translate);
	printf("Percentage zero writes:\t\t\t%.2f\n",  (amount_zero_writes* 1.0f) / amount_accesses);
	printf("Amount of user accesses:\t\t%lu\n", amount_user_accesses);
	printf("Amount big endian:\t\t\t%lu\n", amount_big_endian);
	printf("Amount little endian:\t\t\t%lu\n", amount_little_endian);
	free(tmp_access);
	free(tmp_cr_change);
}


