#include <mpi.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "cachestate.h"
#include <stdlib.h>
#include "coordinator.h"
#include "worker.h"
#include "input_reader.h"



int main(int argc, char** argv) {
	init_cachestate_masks(12, 6);
	int provided;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	if(world_size<= 2) {
		printf("World size needs to be at least 3\n");
		return -1;
	}

	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	int name_len;
	MPI_Get_processor_name(processor_name, &name_len);


	if(argc < 5) {
		printf("First argument should be the path of the pagetable dump\n");
		printf("Second argument specifies the input file location!\n");
		printf("Third argument [yn] specifies wether the input is a pipe!\n");
		printf("Fourth argument specifies the path of the cr3 output file!\n");
		printf("But only %d arguments where provided\n", argc);
		return 1;
	}
	char* yn = argv[3];
	bool input_pipe = (yn[0] == 'y');
	if(world_rank == 0){
		run_coordinator(world_size, argv[1], input_pipe, argv[4]);
	} else if(world_rank == 1) {
		run_input_reader(argv[2], argv[4], input_pipe);
	}else {
		run_worker(world_size-2);
	}
	MPI_Finalize();
	return 0;
}
