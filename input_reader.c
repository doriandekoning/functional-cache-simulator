#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include </usr/include/linux/fcntl.h>

#include "input_reader.h"
#include "cachestate.h"
#include "pipereader.h"
#include "mpi_datatype.h"
#include "config.h"

#define TRACE_EVENT_ID_READ 67
#define TRACE_EVENT_ID_WRITE 69
#define TRACE_EVENT_CR_CHANGE 70


int run_input_reader(char* input_path, char* cr3_path, bool write_cr3_values, bool input_is_file) {
	FILE* input, *cr3_file;
	cache_access* access_buffer;
	cr_change* tmp_cr_change;
	uint8_t next_event_id;
	int cache_access_index;
	int send_success;
	MPI_Request* mpi_request;
	MPI_Status mpi_status;
	bool* first_send;
	char mpi_error_string[MPI_MAX_ERROR_STRING];
	int mpi_error_len;

	MPI_Datatype mpi_access_datatype;
  
  debug_printf("Opening input file at: %s\n", input_path);
	input = fopen(input_path, "r");
	if(!input) {
		printf("Could not open input: %s\n", strerror(errno));
		return 1;
	}
	if(write_cr3_values) {
    debug_print("Writing cr3 values is enabled!\n");
    debug_printf("Opening cr3 values file at: %s\n", cr3_path);
		cr3_file = fopen(cr3_path, "w+");
		if(!cr3_file) {
			printf("Could not open cr3 output file: %s\n", strerror(errno));
      exit(1);
		}
	}
	if(get_mpi_access_datatype(&mpi_access_datatype) != 0){
		printf("Unable to create access datatype!\n");
		return 1;
	}
	printf("Reading header!\n");
	if(input_is_file){
		if(read_header(input) != 0){
			printf("Could not read header!\n");
			return 1;
		}
	}else{
		//Wait for qemu to start 
		while(read_header(input) < 0){
			sleep(1);
		}
	}
	printf("Successfully read header!\n");

	access_buffer = malloc(sizeof(cache_access) * INPUT_BATCH_SIZE);
	tmp_cr_change = malloc(sizeof(cr_change));
//	mpi_request = malloc(sizeof(mpi_request));
//	first_send = malloc(2*sizeof(bool));
//	first_send[0] = true;
//	first_send[1] = true;
	cache_access_index = 0;
	debug_print("Running input reader!\n");
	bool write_cr_values = true;
  int buffer_index = 0;
	while(true) {
	  cache_access* current_access_ptr = &(access_buffer[cache_access_index]);
		next_event_id = get_next_event_id(input);
   	if(next_event_id == (uint8_t)-1) {
	    usleep(100000);//sleep 10 millisec
	    debug_print("Could not get next event id!\n");
			continue;
		}
    debug_printf("Proccessing next event with id:%d\n", next_event_id);
		if(next_event_id == TRACE_EVENT_ID_READ || next_event_id == TRACE_EVENT_ID_WRITE) {
		  if(write_cr_values) {	
			  write_cr_values = false;
			}
      if(get_cache_access(input, &(access_buffer[cache_access_index]))) {
				printf("Could not get cache access:%s \n", strerror(errno));
				return 1;
			}
		}else if(next_event_id == TRACE_EVENT_CR_CHANGE) {
		  if(get_cr_change(input, tmp_cr_change)){
			  printf("Could not read cr change: %s\n", strerror(errno));
			  return 1;
		  }
      if(write_cr_values && tmp_cr_change->register_number == 3){
        debug_printf("Writing CR3 value to file:0x%012lx, %p, %p\n", tmp_cr_change->new_value, &(tmp_cr_change->new_value), cr3_file);
			  if(fwrite(&(tmp_cr_change->new_value), sizeof(tmp_cr_change->new_value), 1, cr3_file) != 1){
				  printf("Unable to write cr3 to file:%lx\n", strerror(errno));
			    return 1;
		    }
			  if(fflush(cr3_file)) {
		      printf("Unable to flush cr3 file!\n");
	  		  return 1;
  			}
        debug_print("Succesfully written cr value to file!\n");
      }else{
  			current_access_ptr->tick = tmp_cr_change->tick;
	  		current_access_ptr->address = tmp_cr_change->new_value;
		  	current_access_ptr->type = CR_UPDATE;//TODO make constant
			  current_access_ptr->cpu = tmp_cr_change->cpu;
				current_access_ptr->size = tmp_cr_change->register_number;
      }
		}else {
		  printf("Unknown eventid!\n");
			return 1;
		}
		if(cache_access_index == INPUT_BATCH_SIZE){
//			int buffer_index = (cache_access_index / INPUT_BATCH_SIZE) - 1;
/*			if(!first_send[buffer_index]) {
				MPI_Wait(&mpi_request[buffer_index], &mpi_status);
				if(mpi_status.MPI_ERROR != MPI_SUCCESS){
					MPI_Error_string(mpi_status.MPI_ERROR, mpi_error_string, &mpi_error_len);
					printf("MPI error occured: %s\n", mpi_error_string);
					return 1;
				}
			}*/
      debug_printf("Sending buffer with %d accesses at: %p to coordinator, with access type: %p\n", INPUT_BATCH_SIZE, access_buffer, mpi_access_datatype);
			send_success = MPI_Send(access_buffer, INPUT_BATCH_SIZE, mpi_access_datatype, 0, 0, MPI_COMM_WORLD);//, &mpi_request[buffer_index] );
			cache_access_index = 0;
			if(send_success != MPI_SUCCESS) {
			 	printf("Unable to send accesses to coordinator!\n");
			  return 1;
			}
    }
    if(!write_cr_values){
  		cache_access_index = ((cache_access_index+1) % (2*INPUT_BATCH_SIZE));
    }
  }
	free(access_buffer);
	free(mpi_request);
	return 0;
}
