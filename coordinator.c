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
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "cachestate.h"
#include "mpi_datatype.h"
#include "pipereader.h"
#include "string.h"
#include "memory.h"
#include "input_reader.h"
#include "pagetable.h"
#include "config.h"

#define PG_MASK (1 << 31)
#define PE_MASK (1 << 0)
#define PAGING_ENABLED(cpu) (control_registers[cpu][0] & PE_MASK) && (control_registers[cpu][0] & PG_MASK)
#define AMOUNT_CONTROL_REGISTERS 9
int* stored_access_counts;

cache_access* stored_accesses;

clock_t start, end;
clock_t time_waited = 0;
clock_t before_waited;

int* cpu_id_map;
uint64_t** control_registers;

uint64_t curCr3 = 0;
FILE* cr3_file;
uint8_t paging_enabled = 0;

int write_cr3_to_file(uint64_t cr3) {
	return fwrite(&cr3, 1, sizeof(cr3), cr3_file) != sizeof(cr3);
}

int read_pagetables(char* path, struct memory* mem) {
	DIR* d = opendir(path);
	struct dirent *dir;
	printf("Reading pagetables in directory:%s\n", path);
	if(d) {
		while((dir = readdir(d)) != NULL) {
			if(!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")){
				continue;
			}
			char fPath[512];
			strcpy(fPath, path);
			strcat(fPath, "/");
			strcat(fPath, dir->d_name);
			if(read_pagetable(mem, fPath)){
				printf("Could not read pagetable:%s\n", &fPath);
				return 1;
			}
		}
		closedir(d);
	}
	return 0;
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

int read_cr_values(char* cr_values_path, uint64_t control_registers[AMOUNT_SIMULATED_PROCESSORS][5]) { //TODO support multiple cpu
	debug_printf("Opening CR Values file at:%s\n", cr_values_path);
	FILE* f = fopen(cr_values_path, "rb");
	if(f == NULL){
		printf("Unable to open control register initial values file:%s\n", strerror(errno));
		return 1;
	}
/*	struct stat buf;
	  stat(cr_values_path, &buf);
  	char* buffer = calloc(sizeof(char), 4096);
	debug_print("Reading cr3 values!\n");
  int buffer_size = fread(buffer,1, buf.st_size, f);
  debug_printf("Read %d characters from file!\n", buffer_size);
  if(buffer_size == 0) {
		perror("Could not read from control registers initial values file!");
		return 1;
	}*/
	uint64_t processor = 0;
  int string_offset = 0;
	for(int i = 0; i < AMOUNT_SIMULATED_PROCESSORS; i++){
    fscanf(f, "%llx\n", &processor);
		processor = map_cpu_id(processor);
    for(int j = 0; j<5; j++) {
       
      fscanf(f, "%llx\n", &(control_registers[processor][j]));
    }
		debug_printf("Paging is: %s\n", PAGING_ENABLED(processor) ? "enabled" : "disabled");
		debug_printf("Initial CR values for processor:%lx\n", processor);
		for(int i = 0; i < 5; i++ ) {
			debug_printf("Processor[%lx]CR%d=0x%lx\n", processor, i, control_registers[processor][i]);
		}
	}
	return 0;
}

int run_coordinator(int world_size, bool read_pgtables, char* input_pagetables, char* cr_values_path) {
	MPI_Datatype mpi_access_type;

  uint64_t control_registers[AMOUNT_SIMULATED_PROCESSORS][5];
	printf("Starting coordinator!\n");
	#ifndef VIRT_TO_PHYS_TRANSLATION
	if(read_pgtables) { printf("Reading pagetables is not available when compiled without virtual to physical address translation!\n"); }
	#endif /* VIRT_TO_PHYS_TRANSLATION */
	signal(SIGINT, sigintHandler);
	unsigned char buf[4048];
	struct memory* mem = init_memory();
	cpu_id_map = malloc(sizeof(int)*AMOUNT_SIMULATED_PROCESSORS);
	for(int i = 0; i < AMOUNT_SIMULATED_PROCESSORS; i++) {
		cpu_id_map[i] = INT_MAX;
	}

	cache_access accesses[sizeof(cache_access)*INPUT_BATCH_SIZE];
	int err = get_mpi_access_datatype(&mpi_access_type);
	if(err != 0) {
		printf("Unable to get mpi access datatype, error:%d\n", err);
		return err;
	}
	//Setup access
	stored_access_counts = calloc(world_size-1, sizeof(int));
	stored_accesses = calloc((world_size) * MESSAGE_BATCH_SIZE, sizeof(cache_access));
	int total_batches = 0;
	int total_packets_stored = 0;
	int amount_packages_read = 0;
	int pagetable_notfound = 0;
	int unable_to_map = 0;
	int total_mem_access = 0;
	int phys_mem_access = 0;
	int total_cr_change = 0;
  MPI_Status status;
	start = clock();
  debug_print("Starting to receive trace events!\n");
  while(true){
    debug_printf("Receiving!%p\n", accesses);
    if(MPI_Recv(accesses, INPUT_BATCH_SIZE, mpi_access_type, 1, 0, MPI_COMM_WORLD, &status) != MPI_SUCCESS) {
      printf("MPI error receiving!\n");
      return 1;
    }
    debug_print("Receveived!\n");
    int count;
    if(MPI_Get_count(&status, mpi_access_type, &count) != MPI_SUCCESS) {
      printf("Unable to get count!\n");
      return 1;
    }
		debug_printf("Received %d accesses\n", count);
    amount_packages_read+=count;
    for(int i = 0; i < count; i++) {
			accesses[i].cpu = map_cpu_id(accesses[i].cpu);
      if(accesses[i].type == CACHE_READ || accesses[i].type == CACHE_WRITE){
        if(total_mem_access == 0){
				  debug_print("Reading initial cr values!\n");
					if(read_cr_values(cr_values_path, control_registers)) {
					  printf("Unable to read initial cr values!\n");
						exit(1);
				  }
					if(read_pagetables(input_pagetables, mem)) {
					  printf("Unable to read pagetables!\n");
						exit(1);
					}
				}
				total_mem_access++;
				//Map cpu to a range from the qemu cpu id to a range
				//1...n
				uint64_t virt = accesses[i].address;
				#ifdef VIRT_TO_PHYS_TRANSLATION
				debug_print("Converting virt to phys!\n");
				if(PAGING_ENABLED(accesses[i].cpu)) {
					accesses[i].address = vaddr_to_phys(mem, control_registers[accesses[i].cpu][3], accesses[i].address);
				}else{
					phys_mem_access++;
				}
				if(unable_to_map % 100000 == 0){
				  printf("%lu out of %lu accesses not mappable: %f\%\n", unable_to_map, total_mem_access, (double)unable_to_map/(double)total_mem_access*100.0);
				}
				if(accesses[i].address == NOTFOUND_ADDRESS ){
          debug_print("unable to map!\n");
					unable_to_map++;
					continue;
				}
				//From here addresses[i].address contains the
				//physical address
				if(total_packets_stored%10000000== 0 ) {
						printf("Percentage of the %lu memory accesses mappable:%.2f\n", total_mem_access, 100.0f*(float)unable_to_map/(float)total_mem_access);
				}
				if(accesses[i].type == CACHE_WRITE) {
					int written = write_sim_memory(mem, accesses[i].address, accesses[i].size, &accesses[i].data);
					if(written != accesses[i].size){
						printf("Error writing to simulated memory!\n");
						return 0;
					}
				}
				#endif /* VIRT_TO_PHYS_TRANSLATION */
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
					printf("Coordinator received %d accesses of which %d where phycical\n", total_mem_access, phys_mem_access);
				}
			//CR CHANG
			#ifdef VIRT_TO_PHYS_TRANSLATION
			}else if(accesses[i].type == CR_UPDATE) {
        debug_print("Received cr update!\n");
				//TODO multiple cpu support
				total_cr_change++;
				if(accesses[i].size > 4 && accesses[i].size != 8) {
					printf("Cr value is not in range (0-4,8): %x!\n", accesses[i].size);
					exit(1);
				}
				//Update current_pagetable
        printf("JHAAAAHE!%lx\n", accesses[i].cpu);
				bool before_paging_enabled = PAGING_ENABLED(accesses[i].cpu);
        printf("HIER!\n");
				control_registers[accesses[i].cpu][accesses[i].size] = accesses[i].address;
        printf("MAAR NIET HIER!\n");
				if(accesses[i].size == 0 && before_paging_enabled != PAGING_ENABLED(accesses[i].cpu)){
					printf("Paging %sabled!\n", PAGING_ENABLED(accesses[i].cpu) ? "en" : "dis"); 
				}
			#endif /* VIRT_TO_PHYS_TRANSLATION*/
			}else{
				printf("Unknown access at tick: %lx type:%d\n",accesses[i].tick, accesses[i].type);
			}
		}
	}
	free(accesses);
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


