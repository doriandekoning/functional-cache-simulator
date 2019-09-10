#include "coordinator.h"
#include "config.h"
#include <mpi.h>

// For mkfifo
#include <sys/types.h>
#include <sys/stat.h>


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "messages/packet.pb-c.h"
#include "varint/varint.h"
#include "cachestate.h"
#include "mpi_datatype.h"
#include "pipereader.h"



int* stored_access_counts;
cache_access* stored_accesses;

clock_t start, end;
clock_t time_waited = 0;
clock_t before_waited;

int* cpu_id_map;

//TODO rank == (worldsize-1) is master

bool store_msg(int worker, cache_access* access) {
    int* index = &stored_access_counts[worker-1];
    cache_access* access_ptr = &stored_accesses[ (((worker-1)*MESSAGE_BATCH_SIZE)  + *index )];
    access_ptr->address = access->address; //TODO map virtual to physical address
	access_ptr->tick = access->tick;
	access_ptr->cpu = access->cpu;
    access_ptr->type = access->type;
    (*index)++;
    return *index >= MESSAGE_BATCH_SIZE;
}

int send_accesses(int worker, MPI_Datatype mpi_access_type) {
    before_waited = clock();
    int success = MPI_Send(&stored_accesses[worker-1], stored_access_counts[worker-1], mpi_access_type, worker, 0, MPI_COMM_WORLD);//TODO
    time_waited += (clock() - before_waited);
    stored_access_counts[worker-1] = 0; //This cannot be done for nonblocking sends

    if(success != MPI_SUCCESS) {
        printf("Unable to send accesses to worker: %d\n", success);
        return 1;
    } //TODO make ISend
    return 0;
}

int map_cpu_id(int cpu_id) {
    for(int i = 0; i < AMOUNT_SIMULATED_PROCESSORS; i++) {
        if(cpu_id_map[i] == cpu_id){
            return i;
        } else if(cpu_id_map[i] == INT_MAX) {
            cpu_id_map[i] = cpu_id;
            return i;
        }
    }
    printf("Cannot map cpu!\n");
    exit(0);
}


int run_coordinator(int world_size, char* input_file) { //TODO rename to pipe
    FILE *pipe;
    unsigned char buf[4048];
    cpu_id_map = malloc(sizeof(int)*AMOUNT_SIMULATED_PROCESSORS); //TODO do mapping on worker
    for(int i = 0; i < AMOUNT_SIMULATED_PROCESSORS; i++) {
        cpu_id_map[i] = INT_MAX;
    }


    MPI_Datatype mpi_access_type;
    int err = get_mpi_access_datatype(&mpi_access_type);
    if(err != 0){
        printf("Unable to create datatype, erorr:%d\n", err);
        return err;
    }
    //Setup access
    stored_access_counts = calloc(world_size-1, sizeof(int));
    stored_accesses = calloc((world_size) * MESSAGE_BATCH_SIZE, sizeof(cache_access));
    pipe = fopen(input_file, "r+");
    if(!pipe) {
        printf("Error occured opening file\n");
        exit(-1);//TODO add status exit codes
    }

    int total_batches = 0;
    int total_packets_stored = 0;
    int amount_packages_read = 0;
    start = clock();

    if(read_header(pipe) != 0) {
        printf("Unable to read header!\n");
        return 2;
    }
    cache_access* tmp_access = malloc(sizeof(cache_access));
    cr3_change* tmp_cr3_change = malloc(sizeof(cr3_change));
    while(true){

	amount_packages_read++;
	int next_eventid = get_next_event_id(pipe);
	if(next_eventid == (uint8_t)-1) {
		printf("Could not get next eventid!\n");
		break;
	}
	if(next_eventid == 67) {
		//Cache access
		if(get_cache_access(pipe, tmp_access)){
			printf("Could not read cache access!\n");
			break;
		}
	        tmp_access->cpu = map_cpu_id(tmp_access->cpu);
		// printf("{addr:%llu,tick:%llu,cpu:%llu,write:%d}\n", tmp_access->address, tmp_access->tick, tmp_access->cpu, tmp_access->write);

		if(amount_packages_read > SIMULATION_LIMIT) {//TODO also in else
			printf("Simulation limit reached%d\n",
			       amount_packages_read);
			break;
		}
	        int worker = (ADDRESS_INDEX(tmp_access->address) % (world_size-1)) + 1; // Add 1 to offset the rank of the coordinator
	        bool shouldSend = store_msg(worker, tmp_access);
	        total_packets_stored++;
	        if(shouldSend) {
	            total_batches++;
	            send_accesses(worker, mpi_access_type);
	        }

	}else if(next_eventid == 68) {
		if(get_cr3_change(pipe, tmp_cr3_change)) {
			printf("Could not read cr3 change!\n");
			break;
		}
		tmp_access->tick = tmp_cr3_change->tick;
		tmp_access->address = tmp_cr3_change->new_cr3;
		tmp_access->cpu = map_cpu_id(tmp_cr3_change->cpu);
		tmp_access->type = CR3_UPDATE;
		total_packets_stored++;
		for(int i = 0; i < world_size -1; i++){//TODO remove duplicate code
			if(store_msg(i+1, tmp_access)) {
				total_batches++;
				send_accesses(i+1, mpi_access_type);
			}
		}
		//printf("New cr3:%lx\n", tmp_cr3_change->new_cr3);
	}else{
		printf("Encountered unknown event: %d\n", next_eventid);
	}
   }
	free(tmp_access);
	free(tmp_cr3_change);
	for(int i = 1; i < world_size; i++) {
		send_accesses(i, mpi_access_type);
	}

    end = clock();
    printf("Time used by coordinator: %f\n", (double)(end-start)/CLOCKS_PER_SEC);
    printf("Time spend waiting for communication: %f\n", (double)time_waited/CLOCKS_PER_SEC);
    printf("Time spend waiting:%f\n", (double)(time_waited/(end-start)));

    printf("Total messages send:\t%d\n", amount_packages_read);
    printf("Total packets stored:\t%d\n", total_packets_stored);
    printf("Total batches send:\t%d\n", total_batches);
    return 0;
}


