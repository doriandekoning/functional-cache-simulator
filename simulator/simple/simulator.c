#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

#ifdef SIMULATE_ADDRESS_TRANSLATION
#ifndef SIMULATE_MEMORY
#error "Not able to simulate address translation without simulating memory"
#endif
#endif

//TODO move each type of reader: printtrace, simplecachesim, simulator to a separate directory
#include "config.h"
#ifdef INPUT_SMEM
#include "filereader/sharedmemreader.h"
#elif INPUT_FILE
#include "filereader/filereader.h"
#endif
#include "cache/state.h"
#include "cache/hierarchy.h"
#include "cache/lru.h"
#include "cache/msi.h"
#include "cache/coherency_protocol.h"
#include "mappingreader/mappingreader.h"
#include "control_registers/control_registers.h"
#include "traceevents.h"

#include "memory/memory.h"

#ifdef SIMULATE_ADDRESS_TRANSLATION
#include "pagetable/pagetable.h"
#endif

#ifdef INPUT_SMEM
#define get_next_event_id smem_get_next_event_id
#define get_memory_access smem_get_memory_access
#define get_tb_start_exec smem_get_tb_start_exec
#define get_invlpg smem_get_invlpg
#define get_cr_change smem_get_cr_change
#elif INPUT_FILE
#define get_next_event_id file_get_next_event_id
#define get_memory_access file_get_memory_access
#define get_tb_start_exec file_get_tb_start_exec
#define get_invlpg file_get_invlpg
#define get_cr_change file_get_cr_change
#endif

#define PG_MASK (1 << 31)
#define PE_MASK (1 << 0)


FILE* out;

struct __attribute__((__packed__)) OutputStruct{
	bool write;
	int64_t timestamp;
	uint64_t address;
};

void cache_miss_func(bool write, uint64_t timestamp, uint64_t address) {
	struct OutputStruct outstruct = {
		.write = write,
		.timestamp = timestamp,
		.address = address,
	};
	fwrite(&outstruct, 1, sizeof(struct OutputStruct), out);
}

int perform_cache_access(uint8_t cpu, uint64_t* address, struct CacheHierarchy* hierarchy, struct Memory* memory, ControlRegisterValues control_register_values, uint64_t timestamp, int type, bool* is_kernel_access) {
	#ifdef SIMULATE_ADDRESS_TRANSLATION
	// printf("Pagingenabled:%d, %d cpu\n", paging_enabled(control_register_values, cpu), cpu);
	if(paging_enabled(control_register_values, cpu)) {
		uint64_t physical_address = 0;
		uint64_t cr3 = get_cr_value(control_register_values, cpu, 3);
		if( get_cr_value(control_register_values, cpu, 4) & (1U << 5)) { //TODO make constant
			physical_address = vaddr_to_phys(memory, cr3 & ~0xFFFUL, *address, false, is_kernel_access);
		}else{
			printf("32 bit lookup!\n");
			physical_address = vaddr_to_phys32(memory, cr3 & ~0xFFFUL, *address, true); //TODO should we sign extend?
		}
		if(physical_address == NOTFOUND_ADDRESS) {//} || physical_address != tmp_access->physaddress)  {
			printf("Not able to translate virt:%lx\n", *address);
			return 1;
		}
		// printf("Virtual address: %lx translated to physical address: %lx\n", address, physical_address);
		*address = physical_address;
	}
	#endif

	#ifdef SIMULATE_CACHE
	return access_cache_in_hierarchy(hierarchy, cpu, *address, timestamp, type);
	#else
	return 0;
	#endif
}

struct CacheHierarchy* setup_cache() {
	struct CacheHierarchy* hierarchy = init_cache_hierarchy(AMOUNT_SIMULATED_PROCESSORS);
	struct CacheLevel* l1 = init_cache_level(AMOUNT_SIMULATED_PROCESSORS, true);
	for(int i = 0; i < AMOUNT_SIMULATED_PROCESSORS; i++) {
		struct CacheState* dcache = setup_cachestate(NULL, true, 8*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
		struct CacheState* icache = setup_cachestate(NULL, true, 8*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
		add_caches_to_level(l1, dcache, icache);
	}
	if(add_level(hierarchy, l1) != 0 ){
		exit(1);
	}
	struct CacheLevel* l2 = init_cache_level(AMOUNT_SIMULATED_PROCESSORS, false);
	for(int i = 0; i < AMOUNT_SIMULATED_PROCESSORS; i++) {
		struct CacheState* state = setup_cachestate(NULL, true, 256*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
		add_caches_to_level(l2, state, NULL);
	}
	if(add_level(hierarchy, l2) != 0 ){
		exit(1);
	}
	struct CacheLevel* l3 = init_cache_level(AMOUNT_SIMULATED_PROCESSORS, false);
	struct CacheState* state = setup_cachestate(NULL, true, 8*1024*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
	add_caches_to_level(l3, state, NULL);
	if(add_level(hierarchy, l3) != 0 ){
		exit(1);
	}
	return hierarchy;
}

#ifdef SIMULATE_ADDRESS_TRANSLATION
int read_pagetables(char* path, struct Memory* mem) {
	DIR* d = opendir(path);
	struct dirent *dir;
	printf("Reading pagetables in directory:%s\n", path);
	if(d) {
		while((dir = readdir(d)) != NULL) {
			if(!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")){
				continue;
			}
			char fPath[512];
			strcpy(fPath, path);
			strcat(fPath, "/");
			strcat(fPath, dir->d_name);
			if(read_pagetable(mem, fPath)){
				printf("Could not read pagetable:%s\n", &fPath);
				return 1;
			}
		}
		closedir(d);
	}
	printf("Succesfully read pagetables!\n");
	return 0;
}
#endif


int main(int argc, char **argv) {
	struct EventIDMapping trace_mapping;
	#ifdef SIMULATE_CACHE
	printf("Simulating cache!\n");
	#endif
	#ifdef SIMULATE_MEMORY
	printf("Simulating memory!\n");
	FILE* memory_backing_file = NULL;
	#endif
	#ifdef SIMULATE_ADDRESS_TRANSLATION
	printf("Simulating address translation!\n");
	#endif

	if(argc < 6) {
		printf("Atleast 5 arguments must be provided, first the mapping path, second the trace file, third output file, fourth cr3 output file\n");
		return 1;
	}
	char* trace_id_mapping = argv[1];
	printf("Trace id mapping file:%s\n", trace_id_mapping);
	char* input_file = argv[2];
	printf("Input file: %s\n", input_file);
	char* output_file = argv[3];
	printf("Output file: %s\n", output_file);
	#ifdef SIMULATE_MEMORY
	char* memory_backing_file_location = NULL;
	if(argc >= 4) {
		memory_backing_file_location = argv[4];
	}
	printf("Memdump file: %s\n", memory_backing_file_location);
	#endif
	char* initial_cr_values_file = NULL;
	if(argc >= 5) {
		initial_cr_values_file = argv[5];
	}
	printf("Initial cr values file: %s\n", initial_cr_values_file);

	#ifdef INPUT_SMEM
	struct shared_mem* in = setup_shared_mem();
	#elif INPUT_FILE
	FILE *in = fopen(input_file, "r");
	if(in == NULL) {
		printf("Could not open input file!\n");
		return 1;
	}
	if(read_header(in) != 0) {
		printf("Unable to read header\n");
		return 1;
	}
	#endif
	unsigned char buf[4048];

	int err = read_mapping(trace_id_mapping, &trace_mapping);
	if(err){
		printf("Could not read trace mapping: %d\n", err);
		return 1;
	}
	print_mapping(&trace_mapping);


	out = fopen(output_file, "w");
	if(out == NULL) {
		printf("Could not open output file: %s!\n", argv[3]);
		return 1;
	}

	struct Memory* simulated_memory;
	struct CacheHierarchy* cache = setup_cache();
	uint64_t mem_range_start = 0;
	// uint64_t mem_range_end = 0x23fffffffULL; 8g
	uint64_t mem_range_end = 1024*1024*1024; //1g
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
	tb_start_exec* tmp_tb_start_exec = malloc(sizeof(tb_start_exec));
	uint64_t delta_t = 0;
	uint64_t current_timestamp = 0;
	uint8_t next_event_id;
	bool negative_delta_t, hit;
	bool paging_is_enabled;
	bool failed = false;
	int amount_unable_to_translate = 0;
	int after_failed = 0;
	uint64_t amount_events = 0;
	bool is_user_page_access;
	#ifdef SIMULATE_MEMORY
	printf("Setting up memory!\n");
	if(memory_backing_file_location != NULL) {
		printf("Opening memory backing file:%s\n", memory_backing_file_location);
		memory_backing_file = fopen(memory_backing_file_location, "r");
		if(memory_backing_file == NULL) {
			printf("Could not open memory backing file!\n");
			return 1;
		}
	}
	simulated_memory = init_memory(memory_backing_file);
	#else
	simulated_memory = init_memory(NULL);
	#endif
	if(initial_cr_values_file != NULL) {
		if(read_cr_values_from_dump(initial_cr_values_file, control_register_values)) {
			printf("Unable to read initial cr register values!\n");
			return 1;
		}
		printf("Succesfully read initial cr values!\n");
	}
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
		amount_events++;
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
			is_user_page_access = false;
			if(perform_cache_access(tmp_access->cpu, &tmp_access->address, cache, simulated_memory, control_register_values, current_timestamp, tmp_access->type, &is_user_page_access)){
				break;
			}

			#ifdef SIMULATE_MEMORY
			//Simulated memory can be seen as write through even if cache isn't (all write  access are directly written to memory)
			if(tmp_access->type == CACHE_EVENT_WRITE && !is_user_page_access
				&& mem_range_start <= tmp_access->address && tmp_access->address < mem_range_end ) {
				if(write_sim_memory(simulated_memory, tmp_access->address, tmp_access->size, &(tmp_access->data)) !=  tmp_access->size) {
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
		} else if(next_event_id == trace_mapping.guest_flush_tlb_invlpg) { //TODO rename trace_mapping to trace_event_mapping
			uint64_t addr;
			uint8_t cpu;
			if(get_invlpg(in, &addr, &cpu)){
				printf("Cannot read invlpg\n");
				break;
			}
			amount_tlb_flush++;
		} else if(next_event_id == trace_mapping.guest_start_exec_tb) {
			if(get_tb_start_exec(in, tmp_tb_start_exec)) {
				printf("Cannot read tb_start_exec\n");
				break;
			}
			#ifdef SIMULATE_CACHE
			int i = 0;
			while(tmp_tb_start_exec->size - (i*CACHE_LINE_SIZE) > -CACHE_LINE_SIZE){
				amount_accesses++;
				uint64_t addr = tmp_tb_start_exec->pc  + (i*CACHE_LINE_SIZE);
				if(perform_cache_access(tmp_tb_start_exec->cpu, &addr, cache, simulated_memory, control_register_values, current_timestamp, CACHE_EVENT_INSTRUCTION_FETCH, &is_user_page_access)){
					break;
				}
				i++;
			}
			#endif
		}else {
			printf("Unknown eventid: %x\n", next_event_id);
			break;
		}

		if(amount_events && (amount_events % 10000000 == 0)) {
			printf("%lu million events processed!\n", amount_events / 1000000);
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
	printf("Amount unable to translate\t\t%lu\n", amount_unable_to_translate);
	printf("Simulated memory size\t\t%lu\n", get_size(simulated_memory));
	free(tmp_access);
	free(tmp_cr_change);
}


