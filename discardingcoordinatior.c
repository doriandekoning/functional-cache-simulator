#include "coordinator.h"
#include "config.h"
#include <mpi.h>

#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>
#include "cachestate.h"
#include "mpi_datatype.h"
#include "pipereader.h"
#include "string.h"
#include "memory.h" //TODO organize imports
#include <unistd.h>
#include <stdio.h>
#include "input_reader.h"



int run_coordinator(int world_size, char* input_pagetables, bool input_is_pipe, char* cr3_file_path, char* something) {

	unsigned char buf[4048];
	struct memory* mem = init_memory();

	MPI_Datatype mpi_access_type;
	cache_access* accesses;
	int err = get_mpi_access_datatype(&mpi_access_type);
	if(err != 0){
		printf("Unable to create datatype, erorr:%d\n", err);
		return err;
	}
	//Setup access
	printf("Opened cr3 file!\n");
	//Wait for header to be available
	cr_change* tmp_cr_change = malloc(sizeof(cr_change));
	accesses = malloc(sizeof(cache_access)*BATCH_SIZE);
	printf("Starting to receive trace events!\n");
	int amount_packages_read = 0;
	while(true){
		amount_packages_read+=BATCH_SIZE;
		if(MPI_Recv(accesses, BATCH_SIZE, mpi_access_type, 1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE) != 0) {
			printf("MPI error receiving!\n");
			return 1;
		}
		for(int i = 0; i < BATCH_SIZE; i++){
			printf("Access:%lu, %d\n",accesses[i].tick, accesses[i].type);
		}
		if(amount_packages_read% 1000000 == 0) {
			printf("%d million packages received!\n", amount_packages_read/1000000);
		}
	}
	free(accesses);
	return 0;
}


