#ifndef COORDINATOR_H
#define COORDINATOR_H

#include <mpi.h>

int run_coordinator(int world_size, MPI_Datatype* mpi_access_type);

#endif /* COORDINATOR_H */
