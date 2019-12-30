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
#include <time.h>


int run_coordinator(int world_size, bool read_pgtables, char* input_pagetables, char* cr_values_path){

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
	accesses = malloc(sizeof(cache_access)*INPUT_READER_BATCH_SIZE);
	printf("Starting to receive trace events!\n");
	int amount_packages_read = 0;
  int batch_index = 0;
  uint64_t max_delta_t = 0;
  uint64_t prev_tick = 0;
  uint64_t prev_write = 0;
  uint64_t below_delta_t[8];
/*  for(int i = 0; i < 8; i++) {
    below_delta_t[i] = 0;
  }*/
  time_t start_time = 0;
  time_t cur_time = 0; 
	while(true){
    if(start_time == 0){
      start_time = time(NULL);
    }
		amount_packages_read+=INPUT_READER_BATCH_SIZE;
		if(MPI_Recv(accesses, INPUT_READER_BATCH_SIZE, mpi_access_type, 1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE) != 0) {
			printf("MPI error receiving!\n");
			return 1;
		}
		for(int i = 0; i < INPUT_READER_BATCH_SIZE; i++){
//			printf("Access:%lu, %d\n",accesses[i].tick, accesses[i].type);		
 //     if(accesses[i].tick > 0 && prev_write > 0 && (accesses[i].tick - prev_write) > max_delta_t) {
   //     max_delta_t = accesses[i].tick - prev_write;
     //   printf("i:%lx[%lx]Larger delta T: %lx current tick: %lx, prev_write: %lx\n", i, accesses[i].type != CR_UPDATE, max_delta_t, accesses[i].tick, prev_write);
     // }
/*     for(int j = 1; j < 8; j++) {
        below_delta_t[j]+= ((accesses[i].tick - prev_write) < (1ULL<<j*8));
     }*/
     // printf("Assigning prev write to:%lx\n", accesses[i].tick);
     // printf("Assigning prev write to:%lx\n", accesses[i].tick);
//     if(prev_write > accesses[i].tick) { 
//         printf("[%d][%d] Type: %d THIS shouldnt happen (i think)%lx, %lx!\n", batch_index, i, accesses[i].type, accesses[i].tick, prev_write);
    //     return 1;
//     }
//      if(accesses[i].cpu > AMOUNT_SIMULATED_PROCESSORS) {
  //      printf("YEA IT HAPPNENED %x\n", accesses[i].cpu);
   //   }
     // prev_write = accesses[i].tick;
    //  printf("[%d][%d]New prevwrite: %lx\n", batch_index, i, prev_write);
    }
		if(amount_packages_read% (10000*INPUT_READER_BATCH_SIZE) == 0) {
      cur_time = time(NULL);
			printf("%d million packages received in %d seconds which is %f Million accesses per second!\n", amount_packages_read/1000000, cur_time - start_time, (amount_packages_read/1000000)/(double)(cur_time-start_time));
      #ifdef PRINT_DELTA_T_FIT
      for(int i = 1; i < 8; i++){
        printf("Amount fits in %d bytes:%lx\n", i, below_delta_t[i]);
      }
      #endif /*PRINT_DELTA_T_FIT*/
		}
    batch_index++;
	}
	free(accesses);
	return 0;
}

