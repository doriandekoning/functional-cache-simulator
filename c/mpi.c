#include <mpi.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "cachestate.h"
#include <stdlib.h>
#include "coordinator.h"
#include "worker.h"




int main(int argc, char** argv) {
	init_cachestate_masks(12, 6);
	MPI_Init(&argc, &argv);

	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	if(world_size<= 1) {
		printf("World size needs to be at least 2\n");
		return -1;
	}

	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	int name_len;
	MPI_Get_processor_name(processor_name, &name_len);


	if(world_rank == 0){
		if(argc < 2) {
			printf("First argument specifies the input file location! But only %d arguments where provided\n", argc);
		}
		run_coordinator(world_size, argv[1]);
	}else {
		run_worker(world_size-1);
	}
	MPI_Finalize();
	return 0;
}
