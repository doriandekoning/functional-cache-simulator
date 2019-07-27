#include <stdio.h>
#include <stdlib.h>

#include "mpi.h"

#define MAX_OPEN_REQUESTS 100

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);


    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    printf("WorldSize:%d, Rank:%d\n",world_size, rank);


    if(rank == 0) {
	    MPI_Request* open_requests = calloc(MAX_OPEN_REQUESTS, sizeof(MPI_Request));
        for(int i = 0; i < MAX_OPEN_REQUESTS; i++) {
            int msg = i + 69;
            MPI_Isend(&msg, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &open_requests[i]);
            // MPI_Status status;
            // MPI_Wait(&open_requests[0], &status);
            // printf("Send:%d\n", i+69);
        }
        printf("Waiting for any!\n");
        int finished;
        MPI_Status status;
        MPI_Waitany(MAX_OPEN_REQUESTS, open_requests, &finished, &status);

        printf("Finished waiting for any!\n");

    } else {
        for(int i = 0; i < MAX_OPEN_REQUESTS; i++ ) {
            int msg;
            MPI_Recv(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("Received:%d\n", msg);

        }
    }

    MPI_Finalize();
    return 0;

}
