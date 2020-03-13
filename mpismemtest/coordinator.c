#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char** argv) {
	int provided, world_size, world_rank, name_len;//TODO rename better
	char *input_path;

	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);


    MPI_Win win;

    // if(rank == 1) {
    //     sleep(1);
    //     MPI_Win_shared_query(win, north, &sz, &northptr);
    // }else{
        uint64_t *buf = malloc(1024 * sizeof(uint64_t));
        for(uint64_t i = 0; i < 1024; i++) {
            buf[i] = i + world_rank;
        }

        MPI_Win_create(&buf, 1024*sizeof(uint64_t), sizeof(uint64_t), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
        sleep(1);
        int read_from = (world_rank+1) % 2;
        MPI_Win_lock( MPI_LOCK_EXCLUSIVE, read_from, 0, win );
        uint64_t vals[3];
        MPI_Get( vals, 3, MPI_UINT64_T, read_from, 0, 3, MPI_UINT64_T, win);
        MPI_Win_unlock( read_from, win );

        printf("[%lx, %lx, %lx]\n", vals[0], vals[1], vals[2]);

        // sleep(50);
        MPI_Win_free(&win);
    // }


	printf("Finished, rank:%d\n", world_rank);
	MPI_Finalize();
	return 0;
}
