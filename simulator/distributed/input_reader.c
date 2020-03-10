#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include </usr/include/linux/fcntl.h>

#include "cache/state.h"
#include "mpi_datatype.h"
#include "config.h"
#include "filereader/sharedmemreader.h"
#include "filereader/filereader.h"
#include "mappingreader/mappingreader.h"
#include "memory/memory_range.h"
#include "control_registers/control_registers.h"

int send_memory_ranges_to_coordinator(char* memory_ranges_file_path, int coordinator_rank) {
	MPI_Datatype memory_range_datatype;
	if(get_mpi_memoryrange_datatype(&memory_range_datatype)) {
		printf("Unable to setup MPI memoryrange datatype!\n");
		return 1;
	}
	// Read memory ranges
	FILE* f = fopen(memory_ranges_file_path, "r");
	if(f == NULL ) {
		printf("Unable to open file!\n");
		return 1;
	}
	printf("[INPUT_READER]Opened memory ranges file: %s\n", memory_ranges_file_path);
	uint64_t amount_memrange = 0;
	struct MemoryRange* memory_ranges = read_memory_ranges(f, &amount_memrange);
	if(amount_memrange <= 0 || memory_ranges == NULL){
		printf("[INPUT_READER]No memory ranges found?!\n");
		return 1;
	}
	debug_printf("[INPUT_READER]%d memory ranges found!\n", amount_memrange);
	struct MemoryRange* memory_ranges_array = malloc(amount_memrange * sizeof(struct MemoryRange));
	struct MemoryRange* cur = memory_ranges;
	for(int i = 0; i < amount_memrange; i++) {
		memory_ranges_array[i].start_addr = cur->start_addr;
		memory_ranges_array[i].end_addr = cur->end_addr;
		printf("AAAAAAAAAAAAAAA[%lx-%lx]\n", cur->start_addr, cur->end_addr);
		cur = cur->next;
	}

	printf("SEnding!!!\n");
	if(MPI_Send(memory_ranges_array, amount_memrange, memory_range_datatype, coordinator_rank, MPI_TAG_MEMRANGE, MPI_COMM_WORLD) != MPI_SUCCESS ){
		printf("Unable to send memory ranges to cooordinator!\n");
		return 1;
	}

	debug_printf("[INPUT_READER]Sent memory ranges to coordinator at rank: :%d\n", coordinator_rank);
	return 0;
}


int run_input_reader(char* input_path, char* mapping_path,  char* memory_range_path, char* initial_cr_values_path, int mpi_world_size, int amount_cpus) {


	struct EventIDMapping trace_mapping;
	int err;
    MPI_Datatype mpi_mapping_datatype;
	uint8_t *buf;

    if(get_mpi_eventid_mapping_datatype(&mpi_mapping_datatype)) {
        printf("Unable to setup MPI datatype!\n");
        return 1;
    }

	if(send_memory_ranges_to_coordinator(memory_range_path, mpi_world_size-2)) {
		return 1;
	}
	#ifdef INPUT_SMEM
	struct shared_mem* in = setup_shared_mem(); //TODO add parameters specifiying base path of shared mem and sems from *input_path* parameter
	#elif INPUT_FILE
	FILE *in = fopen(input_path, "r");
	if(in == NULL) {
		printf("Could not open input file!\n");
		return 1;
	}
	if(file_read_header(in) != 0) {
		printf("Unable to read header\n");
		return 1;
	}
	buf = malloc(SHARED_MEM_BUF_SIZE);
	#endif


	printf("[INPUTREADER]Sending mapping to coordinator!\n");
	err = read_mapping(mapping_path, &trace_mapping);
	if(err){
		printf("Could not read trace mapping: %d\n", err);
		return 1;
	}

	if(MPI_Send(&trace_mapping, 1, mpi_mapping_datatype, mpi_world_size-2, 0, MPI_COMM_WORLD)) {
		printf("Unable to send mapping!\n");
		return 1;
	}
	printf("[INPUTREADER]Sent mapping to coordinator!\n");

	ControlRegisterValues cr_values = init_control_register_values(amount_cpus);
	if(read_cr_values_from_dump(initial_cr_values_path, cr_values, amount_cpus)) {
		printf("Unable to read cr values from file!\n");
		return 1;
	}
	if(MPI_Send(cr_values, AMOUNT_CONTROL_REGISTERS*amount_cpus, MPI_UINT64_T, mpi_world_size-2, MPI_TAG_CONTROLREGISTERVALUES, MPI_COMM_WORLD) != MPI_SUCCESS) {
		printf("Unable to send initial cr values to coordinator!\n");
		return 1;
	}
	printf("[INPUTREADER]Send cr values to coordinator\n");

	int amount_bufs =0;
	while(true) {
		#ifdef INPUT_SMEM
		buf = get_current_buffer(in);
		#elif INPUT_FILE
		if(fread(buf, SHARED_MEM_BUF_SIZE, 1, in) != 1) {
			printf("Unable to read from file!\n");
			return 1;
		}
		#endif
		debug_print("Read input buffer!\n");

		if(MPI_Send(buf, SHARED_MEM_BUF_SIZE, MPI_CHAR, mpi_world_size-2, 1, MPI_COMM_WORLD) != MPI_SUCCESS ){
			printf("Unable to send!\n");
			return 1;
		}
	}

	return 0;
}
