
#ifndef MPI_DATATYPE_H
#define MPI_DATATYPE_H

#include <mpi.h>

#define MPI_TAG_MEMRANGE 54
#define MPI_TAG_CACHE_MISS 55
#define MPI_TAG_CONTROLREGISTERVALUES 56

int get_mpi_memoryrange_datatype(MPI_Datatype* mpi_memoryrange_datatype);
int get_mpi_eventid_mapping_datatype(MPI_Datatype* mpi_mapping_datatype);
int get_mpi_cache_miss_datatype(MPI_Datatype* mpi_cache_miss_datatype);
#endif /* MPI_DATATYPE_H */
