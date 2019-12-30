#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "pipereader.h"
#include "cache/state.h"
#include "mappingreader/mappingreader.h"

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

	in = fopen(argv[2], "r");
	if(in == NULL) {
		printf("Could not open file!\n");
		return 1;
	}
	if(read_header(in) != 0) {
		printf("Unable to read header\n");
		return 1;
	}
	uint64_t cr_update_count = 0;
	uint64_t amount_reads = 0;
	uint64_t amount_writes = 0;
	uint64_t largest_delta_time = 0;
	uint8_t paging_enabled = 0;
	cache_access* tmp_access = malloc(sizeof(cache_access));
	cr_change* tmp_cr_change = malloc(sizeof(cr_change));
	uint64_t delta_t = 0;
	uint64_t current_timestamp = 0;
	uint8_t next_event_id;
	bool negative_delta_t;
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
			if(next_event_id == trace_mapping.guest_mem_load_before_exec) {
				amount_reads++;
			}else{
				amount_writes++;
			}
			if (tmp_access->type == CACHE_WRITE)  {
				printf("Mem write delta_t:\t%lx\t\t at %lx\t\t for address %lx size: %d\tdata:%lx\n", delta_t, current_timestamp, tmp_access->address, tmp_access->size, tmp_access->data);
			}else{
				printf("Mem read delta_t:\t%lx\t\t at %lx\t\t for address %lx size: %d\n", delta_t, current_timestamp, tmp_access->address, tmp_access->size);
			}
		} else if(next_event_id == trace_mapping.guest_update_cr) {
			cr_update_count++;
			if(get_cr_change(in, tmp_cr_change)) {
				printf("Can not read cr change\n");
				break;
			}
			printf("CR[%d] updated to: %lx\n", tmp_cr_change->register_number, tmp_cr_change->new_value);
		}else {
			printf("Unknown eventid: %x\n", next_event_id);
			break;
		}
	}
	printf("Amount of CR updates:\t%lu\n", cr_update_count);
	printf("Reads:\t\t%lu\n", amount_reads);
	printf("Writes:\t\t%lu\n", amount_writes);

	free(tmp_access);
	free(tmp_cr_change);
}


