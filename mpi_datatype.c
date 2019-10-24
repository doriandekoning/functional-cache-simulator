#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>

#include "mpi_datatype.h"
#include "cachestate.h"
#include "pipereader.h"


int get_mpi_access_datatype(MPI_Datatype* mpi_access_type){
	//Setup mpi type for struct access
	int err;
	const int nitems = 6;
	int blocklengths[6] = {1,1,1,1,1,1};
	MPI_Datatype types[6] = {MPI_UINT64_T, MPI_UINT64_T, MPI_UINT64_T, MPI_UINT8_T, MPI_UINT64_T, MPI_UINT8_T};
	MPI_Aint offsets[6];
	offsets[0] = offsetof(cache_access, address);
	offsets[1] = offsetof(cache_access, tick);
	offsets[2] = offsetof(cache_access, cpu);
	offsets[3] = offsetof(cache_access, type);
	offsets[4] = offsetof(cache_access, data);
	offsets[5] = offsetof(cache_access, size);
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
