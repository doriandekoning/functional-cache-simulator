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
#include "pipereader/pipereader.h"
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
	print_mapping(&trace_mapping);
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
	uint64_t amount_user_accesses = 0;
	uint64_t physical_access = 0;
	uint64_t amount_tlb_flush = 0;
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
	int after_failed = 0;
	while(true) {

		if(get_next_event_id(in, &delta_t, &negative_delta_t, &next_event_id) == -1) {
			printf("Unable to read ID of next event!\n");
			break;
		}
		if(!negative_delta_t) {
			current_timestamp += delta_t;
		}else{
			printf("NEGATI(VE!\n");
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
			physical_address = 0;

			#ifdef SIMULATE_ADDRESS_TRANSLATION
			if(paging_enabled(control_register_values, tmp_access->cpu)) {
				uint64_t cr3 = get_cr_value(control_register_values, tmp_access->cpu, 3);
				if( get_cr_value(control_register_values, tmp_access->cpu, 4) & (1U << 5)) { //TODO make constant
					physical_address = vaddr_to_phys(simulated_memory, cr3 & ~0xFFFUL, tmp_access->address, false);

				}else{
					printf("32 bit lookup!\n");
					physical_address = vaddr_to_phys32(simulated_memory, cr3 & ~0xFFFUL, tmp_access->address, true); //TODO should we sign extend?
				}
				if(physical_address == NOTFOUND_ADDRESS) {//} || physical_address != tmp_access->physaddress)  {
					printf("Not able to translate virt:%lx\n", tmp_access->address);
					break;
				}
				debug_printf("Virtual address: %lx translated to physical address: %lx\n", tmp_access->address, physical_address);
			}else{
				physical_access++;
				physical_address = tmp_access->address;
			}
			#endif

			bool write = (tmp_access->type == CACHE_WRITE);
			hit = perform_cache_access(cache_state, 0, physical_address, write); //TODO enable
			if(!hit) {
				amount_miss++;
			}
			#ifdef SIMULATE_MEMORY
			if(tmp_access->address >= 0x2f07ff8 && tmp_access->address < (0x2f07ff8) ) {
				printf("Access write:%d to %lx(%lx), data:%lx, size: %d, current CR3: %lx at loc:%d\n", tmp_access->type == CACHE_WRITE, tmp_access->address, physical_address, tmp_access->data, tmp_access->size, tmp_access->cr3_val, tmp_access->location);
			}
			//Assume write through (all write  access are directly written to memory)
			if(write  && !tmp_access->user_access) {//TODO check if still needed
				if(write_sim_memory(simulated_memory, physical_address,  tmp_access->size, &(tmp_access->data)) !=  tmp_access->size) {
			 		printf("Unable to write memory!\n");
					return 1;
			 	}
			}

			#endif /* SIMULATE_MEMORY */

		} else if(next_event_id == trace_mapping.guest_update_cr) {
			cr_update_count++;
			uint64_t old_cr_val = get_cr_value(control_register_values, tmp_cr_change->cpu, tmp_cr_change->register_number);
			if(get_cr_change(in, tmp_cr_change)) {
				printf("Can not read cr change\n");
				break;
			}
			tmp_cr_change->tick = current_timestamp;

			set_cr_value(control_register_values, tmp_cr_change->cpu, tmp_cr_change->register_number, tmp_cr_change->new_value);
			debug_printf("CR[%d] updated from %016lx to: %016lx at tick: %lu\n", tmp_cr_change->register_number, old_cr_val, tmp_cr_change->new_value, tmp_cr_change->tick);
			bool paging_was_enabled = paging_is_enabled;
			paging_is_enabled = paging_enabled(control_register_values, tmp_access->cpu);
			if(paging_was_enabled != paging_is_enabled) {
				if(paging_is_enabled) {
					printf("Enabled paging and PAE is %s!\n", get_cr_value(control_register_values, tmp_cr_change->cpu, 4) & (1U << 5) ? "enabled" : "disabled");
				}else {
					printf("Disabled paging!\n");
				}
			}
			if(tmp_cr_change->register_number == 4) {
				debug_printf("Change cr4, old:\t%012lx, new: %012lx\n", old_cr_val, tmp_cr_change->new_value);
				debug_printf("Change cr4, old_pae:\t%012lx, new: %012lx\n", (old_cr_val & (1U << 5)), (tmp_cr_change->new_value & (1U << 5)));
				if((old_cr_val & (1U << 5)) != (tmp_cr_change->new_value & (1U << 5))){
					debug_printf("Changed PAE to:%lx\n", tmp_cr_change->new_value & (1U << 5));
					debug_printf("Changed LA57 to:%d\n", tmp_cr_change->new_value & (1U << 12));
					debug_printf("But paging is: %d\n", paging_is_enabled);
				}
			}
		} else if(next_event_id = trace_mapping.guest_flush_tlb_invlpg) { //TODO rename trace_mapping to trace_event_mapping
			uint64_t addr;
			uint8_t cpu;
			if(get_invlpg(in, &addr, &cpu)){
				printf("Cannot read invlpg\n");
				break;
			}
			amount_tlb_flush++;
		}else {
			printf("Unknown eventid: %x\n", next_event_id);
			break;
		}
		if(amount_accesses % 10000000 == 0) {
			printf("%lu million accesses processed!\n", amount_accesses / 1000000);
		}
		if(amount_accesses > SIMULATION_LIMIT){
			printf("Simulation limit reached!\n");
			break;
		}

	}
	printf("Amount of CR updates:\t%lu\n", cr_update_count);
	printf("Reads:\t\t%lu\n", amount_reads);
	printf("Writes:\t\t%lu\n", amount_writes);
	printf("Amount accesses:\t\t\t%lu\n", amount_accesses);
	printf("Amount access miss:\t\t\t%lu\n", amount_miss);
	printf("Amount access hit:\t\t\t%lu\n", amount_accesses-amount_miss);
	printf("Amount physical accesses:\t\t%lu\n", physical_access);
	printf("Amount of user accesses:\t\t%lu\n", amount_user_accesses);
	printf("Amount of tlb flush instructions\t%lu\n", amount_tlb_flush);
	free(tmp_access);
	free(tmp_cr_change);
}


