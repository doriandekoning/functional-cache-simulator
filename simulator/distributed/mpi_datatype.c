#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>

#include <mappingreader/mappingreader.h>

// #include "mpi_datatype.h"
// #include "cache/state.h"
// #include "pipereader.h"


// int get_mpi_access_datatype(MPI_Datatype* mpi_access_type){
// 	//Setup mpi type for struct access
// 	int err;
// 	const int nitems = 6;
// 	int blocklengths[6] = {1,1,1,1,1,1};
// 	MPI_Datatype types[6] = {MPI_UINT64_T, MPI_UINT64_T, MPI_UINT64_T, MPI_UINT8_T, MPI_UINT64_T, MPI_UINT8_T};
// 	MPI_Aint offsets[6];
// 	offsets[0] = offsetof(cache_access, address);
// 	offsets[1] = offsetof(cache_access, tick);
// 	offsets[2] = offsetof(cache_access, cpu);
// 	offsets[3] = offsetof(cache_access, type);
// 	offsets[4] = offsetof(cache_access, data);
// 	offsets[5] = offsetof(cache_access, size);
// 	err = MPI_Type_create_struct(nitems, blocklengths, offsets, types, mpi_access_type);
// 	if(err != 0) {
// 		return err;
// 	}
// 	err = MPI_Type_commit(mpi_access_type);
// 	if(err != 0 ){
// 		return err;
// 	}
// 	return 0;
// }


int get_mpi_eventid_mapping_datatype(MPI_Datatype* mpi_mapping_datatype) {
    int err;
    int blocklengths[6] = {1, 1, 1, 1, 1, 1};
    MPI_Datatype types[6] = {MPI_UINT8_T, MPI_UINT8_T, MPI_UINT8_T, MPI_UINT8_T, MPI_UINT8_T, MPI_UINT8_T};
    MPI_Aint offsets[6];
    offsets[0] = offsetof(struct EventIDMapping, guest_update_cr);
    offsets[1] = offsetof(struct EventIDMapping, instruction_fetch);
    offsets[2] = offsetof(struct EventIDMapping, guest_mem_load_before_exec);
    offsets[3] = offsetof(struct EventIDMapping, guest_mem_store_before_exec);
    offsets[4] = offsetof(struct EventIDMapping, guest_flush_tlb_invlpg);
    offsets[5] = offsetof(struct EventIDMapping, guest_start_exec_tb);
    err = MPI_Type_create_struct(6, blocklengths, offsets, types, mpi_mapping_datatype);
    if(err){
        return err;
    }
    return MPI_Type_commit(mpi_mapping_datatype);
}
