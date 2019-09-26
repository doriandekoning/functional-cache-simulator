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
	int provided;
	printf("THread!\n");
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
//	MPI_Init(&argc, &argv);
	printf("%p\n", argv[argc]);
	printf("TEST!\n");
	MPI_Finalize();
	return 0;
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
		if(argc < 3) {
			printf("First argument should be the path of the pagetable dump\n");
			printf("Second argument specifies the input file location!\n");
			printf("But only %d arguments where provided\n", argc);
			return 1;
		}
		run_coordinator(world_size, argv[1], argv[2]);
	}else {
		run_worker(world_size-1);
	}
	MPI_Finalize();
	return 0;
}
