#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "worker.h"
#include "config.h"
#include "mpi_datatype.h"
#include "cachestate.h"
#include "pagetable.h"


int amount_writebacks = 0;
int amount_misses = 0;
int amount_pagetables = 0;

int run_worker(int amount_workers) {
	MPI_Datatype mpi_access_type;
	int err = get_mpi_access_datatype(&mpi_access_type);
	if(err != 0){
		printf("Unable to create datatype, erorr:%d\n", err);
		return err;
	}
	printf("Allocating a state array of size:%d * %lu\n", AMOUNT_SIMULATED_PROCESSORS * (CACHE_SIZE/amount_workers), sizeof(struct CacheEntry*));
	int* cr3_values = calloc(AMOUNT_SIMULATED_PROCESSORS, sizeof(uint64_t));
	CacheState states = calloc(AMOUNT_SIMULATED_PROCESSORS * (CACHE_SIZE/amount_workers), sizeof(struct CacheEntry*));
	if(states == NULL) {
		printf("Cannot allocate memory for states\n");
		return 0;
	}
	cache_access* messages = malloc(MESSAGE_BATCH_SIZE * sizeof(cache_access));
	uint32_t total_accesses = 0;
	uint32_t total_batches = 0;
	bool last_batch = false;
	do{
		MPI_Status status;//. = malloc(sizeof(MPI_Status));
		int result;
		MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		int count;
		MPI_Get_count(&status, mpi_access_type, &count);
		if (count < MESSAGE_BATCH_SIZE) {
			printf("Finished with %d messages left\n", count);
			last_batch = true;
		} else if(count > MESSAGE_BATCH_SIZE) {
		    printf("Received more than MESSAGE_BATCH_SIZE messages: %d\n", count);
		    exit(1);
		}

		int success = MPI_Recv(messages, count, mpi_access_type, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		if(success != MPI_SUCCESS) {
			printf("Could not receive accesses, error:%d\n", success);
		}
		if(last_batch) {
			printf("Processing last batch!\n");
		}
		total_batches++;
		if(total_batches%1000 == 0){
			printf("Total batches:%d\n", total_batches);
		}
	}while(!last_batch );
	printf("Amount of batches received:\t%u\n", total_batches);
	printf("Amount of accesses:\t%u\n", total_accesses);
	printf("Amount writebacks:\t%d\n", amount_writebacks);
	printf("Amount misses:\t%d\n", amount_misses);
	printf("Amount of pagetables:\t%d\n", amount_pagetables);
	free(states);

	return 0;
}

