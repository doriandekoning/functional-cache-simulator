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

struct cr3_list* cr3s = NULL;

pagetable* find_pagetable_for_cr3(uint64_t cr3) {
	struct cr3_list* curCr3 = cr3s;
	while(curCr3 != NULL) {
		if( curCr3->cr3_value == cr3) {
			return curCr3->pagetable;
		}
		curCr3 = curCr3->next;
	}
	struct cr3_list* newCr3 = calloc(1, sizeof(struct cr3_list));
	newCr3->cr3_value = cr3;
	newCr3->next = cr3s;
	newCr3->pagetable = calloc(1, sizeof(pagetable));
	newCr3->pagetable->levels = 3;
	newCr3->pagetable->addressLength = 54;
	cr3s = newCr3;
	amount_pagetables++;
	return newCr3->pagetable;
}


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
	pagetable* curPagetable = find_pagetable_for_cr3(0); //TODO always 0?
	printf("Allocated initial pagetable!\n");
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
		for(int i = 0; i < count; i++) {
			cache_access* msg = &messages[i];
			if(msg->type == CR3_UPDATE){
				cr3_values[msg->cpu] = msg->address;
				curPagetable = find_pagetable_for_cr3(msg->address);
				continue;
			}
			uint64_t physaddress = vaddr_to_phys(curPagetable, msg->address);
			CacheSetState set_state = get_cache_set_state(states, physaddress, msg->cpu);
			CacheEntryState entry_state = get_cache_entry_state(set_state, physaddress);
			int cur_state = STATE_INVALID;
			if(entry_state != NULL) {
				cur_state = entry_state->state;
			}
			struct statechange state_change = get_msi_state_change(cur_state, msg->type);
			CacheSetState new = apply_state_change(set_state, entry_state, state_change, physaddress);
			new = check_eviction(new);
			set_cache_set_state(states, new, physaddress, msg->cpu);

			total_accesses++;
			// Update state in other cpus
			bool found_in_other_cpu = false;
			for(int i = 0; i < AMOUNT_SIMULATED_PROCESSORS; i++) {
				if(i != msg->cpu) {
					CacheSetState cache_set_state = get_cache_set_state(states, physaddress, i);
					CacheEntryState cache_entry_state = get_cache_entry_state(cache_set_state, physaddress);
					if(cache_entry_state != NULL && cache_entry_state->state != STATE_INVALID) {//TODO check if we need to do this with invalid
						found_in_other_cpu = true;
						bool flush = false;
						struct statechange new_state = get_msi_state_change_by_bus_request(cache_entry_state->state, state_change.bus_request);
						CacheSetState new = apply_state_change(cache_set_state, cache_entry_state, state_change, physaddress);
						new = check_eviction(cache_set_state);
						set_cache_set_state(states, new, physaddress, msg->cpu);
					}
				}
			}

			if(state_change.bus_request == BUS_REQUEST_READ && !found_in_other_cpu) {
				// printf("Miss%llu\n", msg.address);
				amount_misses++;
			}
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

