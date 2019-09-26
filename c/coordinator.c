#include "coordinator.h"
#include "config.h"
#include <mpi.h>

// For mkfifo
#include <sys/types.h>
#include <sys/stat.h>

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <stdbool.h>
#include <time.h>
#include "cachestate.h"
#include "mpi_datatype.h"
#include "pipereader.h"
#include "string.h"
#include "memory.h" //TODO organize imports

int* stored_access_counts;
cache_access* stored_accesses;

clock_t start, end;
clock_t time_waited = 0;
clock_t before_waited;

int* cpu_id_map;
uint64_t* cr;

struct pagetable_list* pagetables; //TODO make cpu dependent
struct pagetable* current_pagetable = NULL;
uint64_t curCr3 = 0;

struct pagetable_list* read_pagetables(char* path) {
	DIR* d = opendir(path);
	struct dirent *dir;	
	if(d) {
		while((dir = readdir(d)) != NULL) {
			if(!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")){
				continue;
			}
			printf("%s\n", dir->d_name);
			uint64_t cr3;
			char fPath[512]; 
			strcpy(fPath, path);
			strcat(fPath, "/");
			strcat(fPath, dir->d_name);
			pagetable* newTable = read_pagetable(fPath, &cr3);
			if(newTable == NULL){
				printf("Could not read pagetable:%s\n", &fPath);
				return NULL;
			}
			printf("%lx\n", cr3);
			struct pagetable_list* newList = malloc(sizeof(struct pagetable_list));
			newList->next = pagetables;
			newList->pagetable = newTable;
			newList->cr3_value =cr3;
			pagetables = newList;
		}
		closedir(d);
	}

	return pagetables;
}

pagetable* find_pagetable_for_cr3(uint64_t cr3) {
        struct pagetable_list* curPagetable = pagetables;
        while(curPagetable != NULL) {
                if( curPagetable->cr3_value == cr3) {
                        return curPagetable->pagetable;
                }
                curPagetable = curPagetable->next;
        }
        return NULL;
}

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


int run_coordinator(int world_size, char* input_pagetables, char* input_file) { //TODO rename to pipe
    FILE *pipe;
    unsigned char buf[4048];
//    struct memory* mem = init_memory();
    cpu_id_map = malloc(sizeof(int)*AMOUNT_SIMULATED_PROCESSORS); //TODO do mapping on worker
    for(int i = 0; i < AMOUNT_SIMULATED_PROCESSORS; i++) {
        cpu_id_map[i] = INT_MAX;
    }
    if(input_pagetables != NULL) {
	pagetables = read_pagetables(input_pagetables);
    }

    cr = malloc(5*sizeof(uint64_t));

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
    int pagetable_notfound = 0;
    int unable_to_map = 0;
    int total_mem_access = 0;
    start = clock();

    if(read_header(pipe) != 0) {
        printf("Unable to read header!\n");
        return 2;
    }
    cache_access* tmp_access = malloc(sizeof(cache_access));
    cr_change* tmp_cr_change = malloc(sizeof(cr_change));
    while(true){

	amount_packages_read++;
	uint8_t next_eventid = get_next_event_id(pipe);
	if(next_eventid == (uint8_t)-1) {
		printf("Could not get next eventid!\n");
		break;
	}
	if(next_eventid == 67 | next_eventid == 69) {
		total_mem_access++;
		//Cache access
		if(get_cache_access(pipe, tmp_access)){
			printf("Could not read cache access!\n");
			break;
		}
	        tmp_access->cpu = map_cpu_id(tmp_access->cpu);
		//TODO check if paging is enabled using cr0 and cr4 values if
		//disabled discard the access for now (should handle it somehow
		//in the future)
		if(current_pagetable == NULL) {
			pagetable_notfound++;
			continue;
		}
//		tmp_access->address = vaddr_to_phys(current_pagetable, tmp_access->address);
//		if(tmp_access->address == 0 ){
//			unable_to_map++;
//			continue;
//		}
/*		if(tmp_access->type == CACHE_WRITE) {
			int written = write(mem, tmp_access->address, tmp_access->size, (uint8_t*)&tmp_access->data);
			if(written != tmp_access->size){
				printf("Error writing to simulated memory!\n");
				return 0;
			}
		}*/
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

	}else if(next_eventid == 70) {
		if(get_cr_change(pipe, tmp_cr_change)) {
			printf("Could not read cr change!\n");
			break;
		}
		if(tmp_cr_change->register_number > 4) {
			printf("Cr value is larger than 4: %x!\n", tmp_cr_change->register_number);
			exit(1);
		}
		//Update current_pagetable
		if(tmp_cr_change->register_number == 3) {
			current_pagetable = find_pagetable_for_cr3(cr[3] & 0x3fffffffff000ULL);

		}
		cr[tmp_cr_change->register_number] = tmp_cr_change->new_value;
	}else{
		printf("Encountered unknown event: %d\n", next_eventid);
	}
	if(unable_to_map != 0 && total_packets_stored%100000 == 0 ) {
	    printf("Total amount of trace events:\t%d\n", amount_packages_read);
	    printf("Total batches send:\t%d\n", total_batches);
	    printf("Pagetable not found: \t%dtimes\n", pagetable_notfound);
	    printf("Unable to map:\t%dtimes\n", unable_to_map);
	    printf("Percentage of memory accesses not mappable:%lu\n", total_mem_access/unable_to_map);
	}
   }
	free(tmp_access);
	free(tmp_cr_change);
	for(int i = 1; i < world_size; i++) {
		send_accesses(i, mpi_access_type);
	}

    end = clock();
    //TODO [MASTER] and [WORKER:index] in front of all printfs
    printf("Time used by coordinator: %f\n", (double)(end-start)/CLOCKS_PER_SEC);
    printf("Time spend waiting for communication: %f\n", (double)time_waited/CLOCKS_PER_SEC);
    printf("Time spend waiting:%f\n", (double)(time_waited/(end-start)));
    printf("Total amount of trace events:\t%d\n", amount_packages_read);
    printf("Total packets stored:\t%d\n", total_packets_stored);
    printf("Total batches send:\t%d\n", total_batches);
    printf("Pagetable not found: \t%dtimes\n", pagetable_notfound);
    printf("Unable to map:\t%dtimes\n", unable_to_map);
    return 0;
}


