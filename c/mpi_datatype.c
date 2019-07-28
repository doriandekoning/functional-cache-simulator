#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>

#include "mpi_datatype.h"
#include "cachestate.h"

int get_mpi_access_datatype(MPI_Datatype* mpi_access_type){
	//Setup mpi type for struct access
	int err;
	const int nitems = 4;
	int blocklengths[4] = {1,1,1,1};
	MPI_Datatype types[4] = {MPI_UINT64_T, MPI_UINT64_T, MPI_UINT64_T, MPI_C_BOOL};
	MPI_Aint offsets[4];
	offsets[0] = offsetof(access, address);
	offsets[1] = offsetof(access, tick);
	offsets[2] = offsetof(access, cpu);
	offsets[3] = offsetof(access, write);
	err = MPI_Type_create_struct(nitems, blocklengths, offsets, types, mpi_access_type);
	if(err != 0) {
		return err;
	}
	err = MPI_Type_commit(mpi_access_type);
	if(err != 0 ){
		return err;
	}
	return 0;
}
