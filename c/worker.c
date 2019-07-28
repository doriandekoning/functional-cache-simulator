#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>

#include "worker.h"
#include "config.h"
#include "mpi_datatype.h"
#include "cachestate.h"



int amount_writebacks = 0;
int amount_misses = 0;

CacheSetState check_eviction(CacheSetState state) {
	int length = list_length(state);
	if(length > ASSOCIATIVITY) {
		if(state->prev != NULL){
			printf("ERROR prev is not NULL");
		}
		struct CacheEntry* lru = get_head(state);
		if(lru->state == STATE_MODIFIED) {
			amount_writebacks++;
		}
		struct statechange change;
		change.new_state = STATE_INVALID;
		return apply_state_change(state, lru, change, 0);
	}
	return state;
}


int run_worker(int amount_workers) {
    MPI_Datatype mpi_access_type;
    int err = get_mpi_access_datatype(&mpi_access_type);
    if(err != 0){
        printf("Unable to create datatype, erorr:%d\n", err);
        return err;
    }
	printf("Allocating a state array of size:%d * %lu\n", AMOUNT_SIMULATED_PROCESSORS * (CACHE_SIZE/amount_workers), sizeof(struct CacheEntry*));

	CacheState states = calloc(AMOUNT_SIMULATED_PROCESSORS * (CACHE_SIZE/amount_workers), sizeof(struct CacheEntry*));
    if(states == NULL) {
        printf("Cannot allocate memory for states\n");
        return 0;
    }
	access* msg = malloc(sizeof(access));
	int total_accesses;
	do{
		MPI_Status status;//. = malloc(sizeof(MPI_Status));
		int result;
		MPI_Probe(0, 0, MPI_COMM_WORLD, &status);
		int count;
		MPI_Get_count(&status, mpi_access_type, &count);

		if (count < MESSAGE_BATCH_SIZE) {
			printf("Finished\n");
			break;
		} else if(count > MESSAGE_BATCH_SIZE) {
            printf("Received more than MESSAGE_BATCH_SIZE messages: %d\n", count);
        }

		int success = MPI_Recv(msg, count, mpi_access_type, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if(success != MPI_SUCCESS) {
            printf("Could not receive accesses, error:%d\n", success);
        }


		total_accesses++;
		uint64_t cache_line = msg->address / CACHE_LINE_SIZE;
		int cache_set_number = get_cache_set_number(cache_line);
		CacheSetState set_state = get_cache_set_state(states, cache_line, msg->cpu);


		CacheEntryState entry_state = get_cache_entry_state(set_state, cache_line);
		int cur_state = STATE_INVALID;
		if(entry_state != NULL) {
			cur_state = entry_state->state;
		}


		struct statechange state_change = get_msi_state_change(cur_state, msg->write);
		CacheSetState new = apply_state_change(set_state, entry_state, state_change, cache_line);
		new = check_eviction(new);
		set_cache_set_state(states, new, cache_line, msg->cpu);

		// Update state in other cpus
		bool found_in_other_cpu = false;
		for(int i = 0; i < AMOUNT_SIMULATED_PROCESSORS; i++) {
			if(i != msg->cpu) {
				CacheSetState cache_set_state = get_cache_set_state(states, cache_line, i);
				CacheEntryState cache_entry_state = get_cache_entry_state(cache_set_state, cache_line);
				if(cache_entry_state != NULL && cache_entry_state->state != STATE_INVALID) {//TODO check if we need to do this with invalid
					found_in_other_cpu = true;
					bool flush = false;
					struct statechange new_state = get_msi_state_change_by_bus_request(cache_entry_state->state, state_change.bus_request);
					CacheSetState new = apply_state_change(cache_set_state, cache_entry_state, state_change, cache_line);
					new = check_eviction(cache_set_state);
					set_cache_set_state(states, new, cache_line, msg->cpu);

				}

			}
		}

		if(state_change.bus_request == BUS_REQUEST_READ && !found_in_other_cpu) {
			// printf("Miss%llu\n", msg.address);
			amount_misses++;
		}


	}while(total_accesses++);

	printf("Amount of accesses:\t%d\n", total_accesses);
	printf("Amount writebacks:\t%d\n", amount_writebacks);
	printf("Amount misses:\t%d\n", amount_misses);

    free(states);

	return 0;
}
