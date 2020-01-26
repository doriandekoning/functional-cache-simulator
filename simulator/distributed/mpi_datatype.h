
#ifndef MPI_DATATYPE_H
#define MPI_DATATYPE_H

#include <mpi.h>


// int get_mpi_access_datatype(MPI_Datatype* mpi_access_type);
int get_mpi_eventid_mapping_datatype(MPI_Datatype* mpi_mapping_datatype);
#endif /* MPI_DATATYPE_H */
