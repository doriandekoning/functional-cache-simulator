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



int run_input_reader(char* input_path, char* mapping_path, char* cr3_path, int mpi_world_size) {
	struct EventIDMapping trace_mapping;
	int err;
    MPI_Datatype mpi_mapping_datatype;
	uint8_t *buf;

    if(get_mpi_eventid_mapping_datatype(&mpi_mapping_datatype)) {
        printf("Unable to setup MPI datatype!\n");
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


	err = read_mapping(mapping_path, &trace_mapping);
	if(err){
		printf("Could not read trace mapping: %d\n", err);
		return 1;
	}
	print_mapping(&trace_mapping);

	if(MPI_Send(&trace_mapping, 1, mpi_mapping_datatype, mpi_world_size-2, 0, MPI_COMM_WORLD)) {
		printf("Unable ot send mapping!\n");
		return 1;
	}

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

		if(MPI_Send(buf, SHARED_MEM_BUF_SIZE, MPI_CHAR, mpi_world_size-2, 1, MPI_COMM_WORLD) != MPI_SUCCESS ){
			printf("Unable to send!\n");
			return 1;
		}
	}

	return 0;
}
