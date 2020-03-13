#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <time.h>

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
#include "memory/memory_range.h"

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
#define read_header file_read_header
#define get_next_event_id file_get_next_event_id
#define get_memory_access file_get_memory_access
#define get_tb_start_exec file_get_tb_start_exec
#define get_invlpg file_get_invlpg
#define get_cr_change file_get_cr_change
#endif

#define PG_MASK (1 << 31)
#define PE_MASK (1 << 0)


char* translated_trace = "/media/ssd/int_trace";

FILE* out;
FILE* translated_out;

struct __attribute__((__packed__)) OutputStruct{
	int64_t timestamp;
	uint64_t address;
	uint8_t write;
	uint8_t size;

};

int amount_misses = 0;
void cache_miss_func(bool write, uint64_t timestamp, uint64_t address) {
	struct OutputStruct outstruct = {
		.timestamp = timestamp,
		.address = address,
		.write = write,
		.size = 8,
	};
	amount_misses++;
	fwrite(&outstruct, 1, sizeof(struct OutputStruct), out);
}

int perform_mem_access(int amount_memories, struct Memory* memories, uint8_t cpu, uint64_t* address, struct CacheHierarchy** hierarchies, ControlRegisterValues control_register_values, uint64_t timestamp, int type, bool* is_user_page) {

	#ifdef SIMULATE_ADDRESS_TRANSLATION
	uint64_t physical_address = 0;
	bool cache_disable = false;
	// printf("Pagingenabled:%d, %d cpu\n", paging_enabled(control_register_values, cpu), cpu);
	if(paging_enabled(control_register_values, cpu)) {
		uint64_t cr3 = get_cr_value(control_register_values, cpu, 3);
		debug_printf("CR3 value using: %lx\n", cr3);
		if( get_cr_value(control_register_values, cpu, 4) & (1U << 5)) { //TODO make constant
			physical_address = vaddr_to_phys(amount_memories, memories, cr3 & ~0xFFFUL, *address, is_user_page, &cache_disable);
		}else{
			printf("32 bit lookup!%d, %lx\n", cpu, cr3);
			exit(1);
			// physical_address = vaddr_to_phys32(memory, cr3 & ~0xFFFUL, *address, true); //TODO should we sign extend?
		}
		if(physical_address == NOTFOUND_ADDRESS) {
			printf("Not able to translate virt:%lx\n", *address);
			return 1;
		}
		*address = physical_address;
		debug_print("Performed virt to phys translation!\n");
	}else{
		printf("Not translating?%lx\n", *address);
	}
	#endif
	#ifdef SIMULATE_CACHE
	if(!cache_disable){
		// Check if address is in range
		for(int i = 0; i < AMOUNT_SIM_CACHES; i++) {
		access_cache_in_hierarchy(hierarchies[i], cpu, *address, timestamp, type);
}
	}
	return 0;
	#else
	(void)type;
	(void)timestamp;
	(void)hierarchy;
	(void)amount_memories;
	(void)memories;
	(void)cpu;
	(void)address;
	(void)is_user_page;
	(void)control_register_values;
	#endif
	#ifdef OUTPUT_TRANSLATION
	uint8_t size = CACHE_LINE_SIZE;
	fwrite(address, 8, 1, translated_out);
	fwrite(&timestamp, 8, 1, translated_out);
	fwrite(&type, 1, 1, translated_out);
	fwrite(&size, 1, 1, translated_out);
	#endif
	return 0;

}


#ifdef SIMPLE_CACHE
struct CacheHierarchy* setup_cache(int amount_cpus) {
printf("Setting up simple cache!\n");
  struct CacheHierarchy* hierarchy = init_cache_hierarchy(amount_cpus);
        struct CacheLevel* l1 = init_cache_level(amount_cpus, true);
        for(int i = 0; i < amount_cpus; i++) {
                struct CacheState* dcache = setup_cachestate(NULL, true, 8*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
                struct CacheState* icache = setup_cachestate(NULL, true, 8*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
                add_caches_to_level(l1, dcache, icache);
        }

        if(add_level(hierarchy, l1) != 0 ){
                exit(1);
        }

        struct CacheLevel* l2 = init_cache_level(amount_cpus, false);
        for(int i = 0; i < amount_cpus; i++) {
                struct CacheState* state = setup_cachestate(NULL, true, 256*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
                add_caches_to_level(l2, state, NULL);
        }
        if(add_level(hierarchy, l2) != 0 ){
                exit(1);
        }
        struct CacheLevel* l3 = init_cache_level(amount_cpus, false);
        struct CacheState* state = setup_cachestate(NULL, true, 8*1024*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
        add_caches_to_level(l3, state, NULL);
        if(add_level(hierarchy, l3) != 0 ){
                exit(1);
        }
        return hierarchy;
/*	struct CacheHierarchy* hierarchy = init_cache_hierarchy(amount_cpus);
	struct CacheLevel* l1 = init_cache_level(1, false);
	struct CacheState* l1_cache = setup_cachestate(NULL, false, 4*1024*1024, 8, 4, &find_line_to_evict_lru, NULL, &cache_miss_func);
	add_caches_to_level(l1, l1_cache, NULL);
	if(add_level(hierarchy, l1) != 0) {
		exit(1);
	}
	return hierarchy;*/
}
#else
struct CacheHierarchy* setup_cache(int amount_cpus) {
	struct CacheHierarchy* hierarchy = init_cache_hierarchy(amount_cpus);
	struct CacheLevel* l1 = init_cache_level(amount_cpus, true);
	for(int i = 0; i < amount_cpus; i++) {
		struct CacheState* dcache = setup_cachestate(NULL, true, 32*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
		struct CacheState* icache = setup_cachestate(NULL, true, 32*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
		add_caches_to_level(l1, dcache, icache);
	}
	if(add_level(hierarchy, l1) != 0 ){
		exit(1);
	}
	struct CacheLevel* l2 = init_cache_level(amount_cpus, false);
	for(int i = 0; i < amount_cpus; i++) {
		struct CacheState* state = setup_cachestate(NULL, true, 256*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
		add_caches_to_level(l2, state, NULL);
	}
	if(add_level(hierarchy, l2) != 0 ){
		exit(1);
	}
	struct CacheLevel* l3 = init_cache_level(amount_cpus, false);
	struct CacheState* state = setup_cachestate(NULL, true, 8*1024*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
	add_caches_to_level(l3, state, NULL);
	if(add_level(hierarchy, l3) != 0 ){
		exit(1);
	}
	return hierarchy;
}
#endif

struct CacheHierarchy* setup_empty_cache(int amount_cpus) {
	return init_cache_hierarchy(amount_cpus);
}



int main(int argc, char **argv) {
        time_t start_t = time(NULL);
	struct EventIDMapping trace_mapping;
	printf("Cache simulator started!\n");
	#ifdef SIMULATE_CACHE
	printf("Simulating cache!\n");
	#else
	printf("Not simulating cache!\n");
	#endif
	#ifdef SIMULATE_MEMORY
	printf("Simulating memory!\n");
	#else
	printf("Not simulating memory!\n");
	#endif
	#ifdef SIMULATE_ADDRESS_TRANSLATION
	printf("Simulating address translation!\n");
	#else
	printf("Not simulating address translation!\n");
	#endif

	if(argc < 7) {
		printf("Atleast 6 arguments must be provided, first the mapping path, second the trace file, third output file, fourth cr3 output file\n");
		return 1;
	}
	char* trace_id_mapping = argv[1];
	printf("Trace id mapping file:%s\n", trace_id_mapping);
	char* input_file = argv[2];
	printf("Input file: %s\n", input_file);
	char* output_file = argv[3];
	printf("Output file: %s\n", output_file);
	int amount_simulated_processors = atoi(argv[4]);
	printf("Simulating %d processors!\n", amount_simulated_processors);

	#ifdef SIMULATE_MEMORY
	char* memory_backing_file_location = NULL;
	char* memory_ranges_file_location = NULL;
	if(argc >= 6) {
		memory_backing_file_location = argv[5];
		memory_ranges_file_location = argv[6];
	}
	printf("Memdump file: %s\n", memory_backing_file_location);
	printf("Memranges file: %s\n", memory_ranges_file_location);
	#endif
	char* initial_cr_values_file = NULL;
	if(argc >= 7) {
		initial_cr_values_file = argv[7];
	}
	printf("Initial cr values file: %s\n", initial_cr_values_file);

	#ifdef INPUT_SMEM
	struct shared_mem* in = setup_shared_mem();
	#elif INPUT_FILE
	FILE *in = fopen(input_file, "rb");
	if(in == NULL) {
		printf("Could not open input file!\n");
		return 1;
	}
	if(read_header(in) != 0) {
		printf("Unable to read header\n");
		return 1;
	}
	#endif

	int err = read_mapping(trace_id_mapping, &trace_mapping);
	if(err){
		printf("Could not read trace mapping: %d\n", err);
		return 1;
	}
	print_mapping(&trace_mapping);


	out = fopen(output_file, "wb");
	if(out == NULL) {
		printf("Could not open output file: %s!\n", argv[3]);
		return 1;
	}
	uint64_t cr_update_count = 0;
	uint64_t amount_reads = 0;
	uint64_t amount_writes = 0;
	uint64_t amount_accesses = 0;
	uint64_t amount_miss = 0;
	uint64_t physical_access = 0;
	uint64_t amount_tlb_flush = 0;
	ControlRegisterValues control_register_values = init_control_register_values(amount_simulated_processors);
	cache_access* tmp_access = malloc(sizeof(cache_access));
	cr_change* tmp_cr_change = malloc(sizeof(cr_change));
	tb_start_exec* tmp_tb_start_exec = malloc(sizeof(tb_start_exec));
	uint64_t delta_t = 0;
	uint64_t current_timestamp = 0;
	uint8_t next_event_id;
	bool negative_delta_t;
	bool paging_is_enabled = false;
	int amount_unable_to_translate = 0;
	uint64_t amount_events = 0;
	#ifdef SHOW_MAPS
	time_t start_time = time(NULL);
	#endif
	#ifdef SIMULATE_MEMORY
	
//	struct CacheHierarchy* cache = setup_cache(amount_simulated_processors);
	struct CacheHierarchy** caches = malloc(sizeof(struct CacheHierarchy*) * AMOUNT_SIM_CACHES);
        for(int i = 0; i < AMOUNT_SIM_CACHES; i++) {
		caches[i] = setup_cache(amount_simulated_processors);
        }
	bool is_user_page_access;
	uint amount_simulated_memories = 0;
	struct Memory* memories = NULL;
	printf("Setting up memory!\n");


	if(memory_ranges_file_location != NULL) {
		printf("Opening memory ranges file:%s\n", memory_ranges_file_location);
		FILE* memory_ranges_file = fopen(memory_ranges_file_location, "r");
		if(memory_ranges_file == NULL) {
			printf("Could not open memory ranges file!\n");
			return 1;
		}
		debug_print("Opened memory ranges file!\n");
		debug_print("Setting up memory!\n");
		memories = malloc(10*sizeof(struct Memory));
		debug_print("Allocated memory structs!\n");
		debug_print("Reading memory ranges!\n");
		uint64_t amount_memranges;
		struct MemoryRange* next = read_memory_ranges(memory_ranges_file, &amount_memranges);
		debug_printf("Read memory ranges:%p\n", next);
		int i = 0;
		while(next != NULL) {
			debug_printf("Next:%p\n", next);
			debug_printf("Initializing memory for range: [%lx, %lx]\n", next->start_addr, next->end_addr);
			debug_printf("Allocating bfile memory:%d bytes\n", strlen(memory_backing_file_location) + 16);
			char* bfile_path = calloc(1, strlen(memory_backing_file_location) + 17);
			debug_printf("Allocated bfile path memory:%p\n", bfile_path);
			debug_printf("Creating memory backing file location for address:%lx with base: %s\n",next->start_addr, memory_backing_file_location);
			sprintf(bfile_path, "%s-%lx", memory_backing_file_location, next->start_addr);
			debug_printf("Memory backing file location:%s\n", bfile_path);
			FILE* bfile = fopen(bfile_path, "r");
			printf("Backing file [%s]:%p\n", bfile_path, bfile);
			if(bfile == NULL) {
				printf("Unable to open bfile!\n");
				exit(1);
			}
			free(bfile_path);
			memories[i] = *init_memory(bfile, next->start_addr, next->end_addr);
			i++;
			if(i >= 10){
				printf("Maximum amount of memory ranges exceeded!\n");
				exit(1);
			}
			next = next->next;
		}
		amount_simulated_memories = i;
	}
	#endif

	#ifdef OUTPUT_TRANSLATION
	translated_out = fopen(translated_trace, "wb");
	if(translated_out == NULL ){
		printf("Unable to open intermediate trace file!\n");
	}
	printf("Using intermediate trace file: %s\n", translated_trace);
	#endif



	if(initial_cr_values_file != NULL) {
		if(read_cr_values_from_dump(initial_cr_values_file, control_register_values, amount_simulated_processors)) {
			printf("Unable to read initial cr register values!\n");
			return 1;
		}
		printf("Succesfully read initial cr values!\n");
	}

  uint64_t user_access = 0;
  uint64_t kernel_access = 0;
  uint64_t amount_fetches = 0;

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
			#ifdef SIMULATE_MEMORY
			is_user_page_access = false;
			if(perform_mem_access(amount_simulated_memories, memories, tmp_access->cpu, &tmp_access->address, caches, control_register_values, current_timestamp, tmp_access->type, &is_user_page_access)){
				break;
			}
			if(is_user_page_access){
				user_access++;
			}else{
				kernel_access++;
			}
			#ifdef INTERMEDIATE_TRACE
			fwrite(&tmp_access->tick, 8, 1, intermediate_trace_file);
			fwrite(&tmp_access->address, 8, 1, intermediate_trace_file);
			fwrite(&tmp_access->size, 1, 1, intermediate_trace_file);
			fwrite(&tmp_access->type, 1, 1, intermediate_trace_file);
			#endif

			//Simulated memory can be seen as write through even if cache isn't (all write  access are directly written to memory)
			if(tmp_access->type == CACHE_EVENT_WRITE && !is_user_page_access) {
				 write_sim_memory(amount_simulated_memories, memories, tmp_access->address, tmp_access->size, &tmp_access->data);
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
					debug_printf("Enabled paging and PAE is %s!\n", get_cr_value(control_register_values, tmp_cr_change->cpu, 4) & (1U << 5) ? "enabled" : "disabled");
				}else {
					debug_printf("Disabled paging!\n");
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
			int i = 0;
		 //printf("TBSize:%u lines %u\n", tmp_tb_start_exec->size / CACHE_LINE_SIZE, tmp_tb_start_exec->size);
			while(tmp_tb_start_exec->size - (i*CACHE_LINE_SIZE) > 0){
				amount_accesses++;
        amount_fetches++;
				#ifdef SIMULATE_MEMORY
				uint64_t addr = tmp_tb_start_exec->pc  + (i*CACHE_LINE_SIZE);
				if(perform_mem_access(amount_simulated_memories, memories, tmp_tb_start_exec->cpu, &addr, caches, control_register_values, current_timestamp, CACHE_EVENT_INSTRUCTION_FETCH, &is_user_page_access)){
					break;
				}
				#endif
				i++;
			}
		}else {
			printf("Unknown eventid: %x\n", next_event_id);
			break;
		}
		if(amount_events && (amount_events % 100000000 == 0)) {
			#ifdef PRINT_MEMSIZE
			uint64_t total_size = 0;
			for(uint i = 0; i < amount_simulated_memories; i++) {
				total_size += get_size(&memories[i]);
			}
			printf("%lu million events processed\tmemory size:%lu!\n", amount_events / 1000000, total_size / (1024*1024));
			#else
			printf("%lu million events processed!\n", amount_events / 1000000);
			#endif

		}
		#ifdef SHOW_MAPS
		if(amount_events && (amount_events % 1000000000 == 0)) {
			printf("Time for previous billion: %lu\n", time(NULL) - start_time);
			start_time = time(NULL);
		}
		#endif
		if(amount_events > SIMULATION_LIMIT){
			printf("Simulation limit reached! %lu\n", amount_events);
			break;
		}

	}
	printf("Amount of CR updates:\t%lu\n", cr_update_count);
	printf("Reads:\t\t%lu\n", amount_reads);
	printf("Writes:\t\t%lu\n", amount_writes);
  printf("Fetch:\t\t%lu\n", amount_fetches);
	printf("Amount accesses:\t\t\t%lu\n", amount_accesses);
	printf("Amount access miss:\t\t\t%lu\n", amount_miss);
	printf("Amount access hit:\t\t\t%lu\n", amount_accesses-amount_miss);
	printf("Amount physical accesses:\t\t%lu\n", physical_access);
	printf("Amount of tlb flush instructions\t%lu\n", amount_tlb_flush);
	printf("Amount unable to translate\t\t%u\n", amount_unable_to_translate);
	printf("Amount cache missess\t\t%u\n", amount_misses);
  	printf("Amount kernel accesses\t\t%lu\n", kernel_access);
  	printf("Amount user accesses\t\t%lu\n", user_access);
	#ifdef SIMULATE_MEMORY
	#endif
	#ifdef INPUT_FILE
	int bytes_read =  ftell(in);
	printf("In total %u bytes where read from the input file\n", bytes_read);
	printf("This is approximately %u Mb\n", bytes_read / (1024*1024));
	#endif
	printf("Seconds spend: %ld\n", time(NULL) - start_t);
	#ifdef SIMULATE_MEMORY
	uint64_t total_size = 0;
	for(uint i = 0; i < amount_simulated_memories; i++) {
		total_size += get_size(&memories[i]);
	}
	printf("In total the memory used:%lu MB\n", total_size / (1024*1024));
	#endif
	free(tmp_access);
	free(tmp_cr_change);
}





