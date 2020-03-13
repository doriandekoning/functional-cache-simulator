#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "filereader/filereader.h"
#include "filereader/sharedmemreader.h"
#include "cache/state.h"
#include "mappingreader/mappingreader.h"

#define PG_MASK (1 << 31)
#define PE_MASK (1 << 0)


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


int main(int argc, char **argv) {
	struct EventIDMapping trace_mapping;
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
	#ifdef INPUT_SMEM
	struct shared_mem* in = setup_shared_mem();
	#else
	FILE* in = fopen(argv[2], "r");
	if(in == NULL) {
		printf("Could not open file!\n");
		return 1;
	}
	if(file_read_header(in) != 0) {
		printf("Unable to read header\n");
		return 1;
	}
	#endif

	uint64_t cr_update_count = 0;
	uint64_t amount_reads = 0;
	uint64_t amount_writes = 0;
	uint64_t amount_fetches = 0;
	uint64_t total_event_count = 0;
	uint64_t largest_delta_time = 0;
	uint8_t paging_enabled = 0;
	cache_access* tmp_access = malloc(sizeof(cache_access));
	cr_change* tmp_cr_change = malloc(sizeof(cr_change));
	tb_start_exec* tmp_tb_start_exec = malloc(sizeof(tb_start_exec));
	uint64_t delta_t = 0;
	uint64_t current_timestamp = 0;
	uint8_t next_event_id;
	bool negative_delta_t;
	print_mapping(&trace_mapping);
  int cur_cpu = -1;
  int amount_cur_cpu = 0;
	while(true) {
		total_event_count++;
		if(total_event_count && total_event_count % 10000000  == 0){
			printf("Processed: %lu mil events\n", total_event_count  /1000000);
			printf("Amount of CR updates:\t%lu\n", cr_update_count);
			printf("Reads:\t\t%lu\n", amount_reads);
			printf("Writes:\t\t%lu\n", amount_writes);
			printf("Fetched:\t\t%lu\n", amount_fetches);
      printf("Bytes read:\t\t%lu\n", ftell(in));
		}

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
      if(tmp_access->cpu == cur_cpu) {
          if(amount_cur_cpu%100 == 0) {
//              printf("Amount cur cpu:%d\n", amount_cur_cpu);
          }
      }else{
          cur_cpu = tmp_access->cpu;
          amount_cur_cpu = 0;
      }
      amount_cur_cpu++;
			if(next_event_id == trace_mapping.guest_mem_load_before_exec) {
				amount_reads++;
			}else{
				amount_writes++;
			}
		} else if(next_event_id == trace_mapping.guest_update_cr) {
			cr_update_count++;
			if(get_cr_change(in, tmp_cr_change)) {
				printf("Can not read cr change\n");
				break;
			}
			// printf("CR[%d] updated to: %lx\n", tmp_cr_change->register_number, tmp_cr_change->new_value);
		}else if(next_event_id == trace_mapping.guest_start_exec_tb) {
			if(get_tb_start_exec(in, tmp_tb_start_exec)) {
				printf("Cannot read tb_start_exec\n");
				break;
			}

			int i = 0;
			while(tmp_tb_start_exec->size - (i*CACHE_LINE_SIZE) > CACHE_LINE_SIZE){
				amount_fetches++;
				i++;
			}
		}else {
			printf("Unknown eventid: %x\n", next_event_id);
			break;
		}

	}


	free(tmp_access);
	free(tmp_cr_change);
}




