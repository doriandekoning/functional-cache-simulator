#include <mpi.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "cachestate.h"
#include <stdlib.h>
#include "coordinator.h"
#include "worker.h"


MPI_Datatype mpi_access_type;



int main(int argc, char** argv) {

	MPI_Init(&argc,&argv);

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
	//Setup mpi type for struct access
	const int nitems = 4;
	int blocklengths[4] = {1,1,1,1};
	MPI_Datatype types[4] = {MPI_UINT64_T, MPI_UINT64_T, MPI_UINT64_T, MPI_C_BOOL};
	MPI_Aint offsets[4];
	offsets[0] = offsetof(access, address);
	offsets[1] = offsetof(access, tick);
	offsets[2] = offsetof(access, cpu);
	offsets[3] = offsetof(access, write);
	MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_access_type);
	MPI_Type_commit(&mpi_access_type);

	if(world_rank == 0){
		run_coordinator(world_size, &mpi_access_type);
	}else {
		run_worker(world_size-1, &mpi_access_type);
	}
	MPI_Finalize();
	return 0;
}

