#include "coordinator.h"
#include "config.h"
#include <mpi.h>
#include <time.h>


#include <signal.h>
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
FILE* cr3_file;
uint64_t* cr;

int write_cr3_to_file(uint64_t cr3) {
	return fwrite(&cr3, 1, sizeof(cr3), cr3_file) != sizeof(cr3);
}

struct pagetable_list* read_pagetables(char* path) {
	DIR* d = opendir(path);
	struct dirent *dir;	
	if(d) {
		while((dir = readdir(d)) != NULL) {
			if(!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")){
				continue;
			}
			printf("Reading pagetable: %s\n", dir->d_name);
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
    printf("Unable to map cpu:%lx\n", cpu_id);
    exit(0);
}


void sigintHandler(int sig) {
	printf("%d recevied\n", sig);
}

int run_coordinator(int world_size, char* input_pagetables, bool input_is_pipe, char* cr3_file_path) {
	signal(SIGINT, sigintHandler);
	unsigned char buf[4048];
	struct memory* mem = init_memory();
	cpu_id_map = malloc(sizeof(int)*AMOUNT_SIMULATED_PROCESSORS);
	for(int i = 0; i < AMOUNT_SIMULATED_PROCESSORS; i++) {
		cpu_id_map[i] = INT_MAX;
	}

	cr = malloc(5*sizeof(uint64_t));

	MPI_Datatype mpi_access_type;
	cache_access* accesses;
	int err = get_mpi_access_datatype(&mpi_access_type);
	if(err != 0){
		printf("Unable to create datatype, erorr:%d\n", err);
		return err;
	}
	//Setup access
	stored_access_counts = calloc(world_size-1, sizeof(int));
	stored_accesses = calloc((world_size) * MESSAGE_BATCH_SIZE, sizeof(cache_access));
/*	cr3_file = fopen(cr3_file_path, "r");
	if(!cr3_file) {
		printf("Unable to open cr3 file \n");
		exit(-2);
	}*/
	printf("Opened cr3 file!\n");
	int total_batches = 0;
	int total_packets_stored = 0;
	int amount_packages_read = 0;
	int pagetable_notfound = 0;
	int unable_to_map = 0;
	int total_mem_access = 0;
	int total_cr_change = 0;
	start = clock();
	//Wait for header to be available
	cr_change* tmp_cr_change = malloc(sizeof(cr_change));
	accesses = malloc(sizeof(cache_access)*BATCH_SIZE);
	cr = calloc(9, sizeof(uint64_t));
	printf("Starting to receive trace events!\n");
	while(true){
		amount_packages_read++;
		if(MPI_Recv(accesses, BATCH_SIZE, mpi_access_type, 1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE) != 0) {
			printf("MPI error receiving!\n");
			return 1;
		}
		for(int i = 0; i < BATCH_SIZE; i++) {
			if(accesses[i].type == CACHE_READ || accesses[i].type == CACHE_WRITE){
				total_mem_access++;
				//Map cpu to a range from the qemu cpu id to a range
				//1...n
				accesses[i].cpu = map_cpu_id(accesses[i].cpu);

				//TODO map virt to phys (if paging is enabled)
	/*			if(current_pagetable == NULL) {
					pagetable_notfound++;
					continue;
				}
				tmp_access->address = vaddr_to_phys(current_pagetable, tmp_access->address);
				if(tmp_access->address == 0 ){
					unable_to_map++;
					continue;
				}
				if(unable_to_map != 0 && total_packets_stored%10000000== 0 ) {
					printf("Percentage of the %lu memory accesses mappable:%.2f\n", total_mem_access, 100.0f*(float)unable_to_map/(float)total_mem_access);
				}*/

				if(accesses[i].type == CACHE_WRITE) {
					int written = write_sim_memory(mem, accesses[i].address, accesses[i].size, (uint8_t*)&(accesses[i].data));
					if(written != accesses[i].size){
						printf("Error writing to simulated memory!\n");
						return 0;
					}
				}
				if(amount_packages_read > SIMULATION_LIMIT){
					printf("Simulation limit reached!\n");
					return 0;
				}

				int worker = (ADDRESS_INDEX(accesses[i].address) % (world_size-2)) + 2; // Add 2 to offset the rank of the coordinator and reader
				bool shouldSend = store_msg(worker, &(accesses[i]));
				total_packets_stored++;
				if(shouldSend) {
					total_batches++;
					send_accesses(worker, mpi_access_type);
				}
				if(total_mem_access %1000000 == 0) {
					printf("Coordinator received %d accesses\n", total_mem_access);
				}
			//CR CHANGE
			}else{

				//TODO multiple cpu support
				total_cr_change++;
				if(accesses[i].size > 4 && accesses[i].size != 8) {
					printf("Cr value is not in range (0-4,8): %x!\n", accesses[i].size);
					exit(1);
				}
				//Update current_pagetable
				cr[tmp_cr_change->register_number] = accesses[i].address;
				if(accesses[i].size == 3) {
					current_pagetable = find_pagetable_for_cr3(cr[3] & 0x3fffffffff000ULL);
				}
			}
		}
	}
	free(accesses);
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


