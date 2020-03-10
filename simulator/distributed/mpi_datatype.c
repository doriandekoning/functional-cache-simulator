#ifdef USING_MPI
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>

#include <mappingreader/mappingreader.h>

#include "memory/memory_range.h"
#include "simulator/distributed/output.h"



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

int get_mpi_memoryrange_datatype(MPI_Datatype* mpi_memoryrange_datatype) {
    int blocklenghts[2] = {8, 8};
    MPI_Datatype types[2] = {MPI_UINT64_T, MPI_UINT64_T};
    MPI_Aint offsets[2] = {offsetof(struct MemoryRange, start_addr), offsetof(struct MemoryRange, start_addr)};
    if(MPI_Type_create_struct(2, blocklenghts, offsets, types, mpi_memoryrange_datatype)) {
        return 1;
    }
    return MPI_Type_commit(mpi_memoryrange_datatype);
}


int get_mpi_cache_miss_datatype(MPI_Datatype* mpi_cache_miss_datatype) {
    int blocklengths[3] = {1, 1, 1};
    MPI_Datatype types[3] = {MPI_UINT64_T, MPI_UINT64_T, MPI_UINT8_T};
    MPI_Aint offsets[3] = {offsetof(struct CacheMiss, addr), offsetof(struct CacheMiss, timestamp), offsetof(struct CacheMiss, cpu)};
    if(MPI_Type_create_struct(3, blocklengths, offsets, types, mpi_cache_miss_datatype)){
        return 1;
    }
    return MPI_Type_commit(mpi_cache_miss_datatype);
}

#endif /*MPI*/
