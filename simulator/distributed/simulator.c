#include <mpi.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "config.h"
#include "coordinator.h"
#include "worker.h"
#include "simulator/distributed/input_reader.h"
// #include "simulator/distributed/mpi_datatype.h"

#include "cache/state.h"


int main(int argc, char** argv) {
	int provided, world_size, world_rank, name_len;//TODO rename better
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	char *mapping_path, *initial_cr_values_path, *cr3_input_path, *input_path;

	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	if(world_size<= 2) {
		printf("World size needs to be at least 3\n");
		return -1;
	}
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Get_processor_name(processor_name, &name_len);


	if(argc < 4) {
		printf("At least 2 arguments need to be specified:\n");
		printf("Argument 1: trace event id mapping path!\n");
		printf("Argument 2: the initial cr values path!\n");
		printf("Argument 3: input path!\n");
		return 1;
	}
	mapping_path = argv[1];
	cr3_input_path = argv[2];
	input_path = argv[3];

	if(world_rank == (world_size - 2)){
		if(run_coordinator(world_size)) {
			return 1;
		}
	} else if(world_rank == (world_size -1)) {
		if( run_input_reader(input_path, mapping_path, cr3_input_path, world_size)){
			return 1;
		}
	}else {
		if(run_worker(world_size, world_rank)) {
			return 1;
		}
	}
	printf("Finished, rank:%d\n", world_rank);
	MPI_Finalize();
	return 0;
}
