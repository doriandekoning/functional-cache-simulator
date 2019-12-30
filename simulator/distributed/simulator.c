#include <mpi.h>
#include <string.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "cache/state.h"
#include <stdlib.h>
#include "coordinator.h"
#include "worker.h"
#include "input_reader.h"



int main(int argc, char** argv) {
	int provided, world_size, world_rank, name_len;//TODO rename better
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	char *pg_tables_dump_path, *cr3_output_path, *input_path, *mapping_path, *initial_cr_values_path;
	bool read_pgtables, input_is_file;

	init_cachestate_masks(12, 6);
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	if(world_size<= 2) {
		printf("World size needs to be at least 3\n");
		return -1;
	}
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Get_processor_name(processor_name, &name_len);

	if(argc < 5) {
		printf("At least 4 arguments need to be specified:\n");
		printf("First argument specifies wether to read pagetables [y/*]\n");
		printf("Second argument specifies wether to the input is a file (or a pipe) [y/*]\n");
		printf("Third argument specifies the input file path\n");
		printf("Fourth argument specifies the mapping file path\n");
		return 1;
	}
	read_pgtables = (!strcmp(argv[1], "y"));
	input_is_file = (!strcmp(argv[2], "y"));
	input_path = argv[3];
	mapping_path= argv[4];
	if(read_pgtables) {
		if(argc < 6) {
			printf("When reading pagetables 3 additional arguments are needed:\n");
			printf("Fourth argument specifies the page table dump directory!\n");
			printf("Fifth argument specifes the path of the output file with all encountered cr3 values!\n");
			printf("Sixt argument specifies the path of the path of the initial CR values file!\n");
			return 1;
		}
		pg_tables_dump_path = argv[5];
		cr3_output_path = argv[6];
		initial_cr_values_path = argv[7];
		if(world_rank == 0) {
		debug_printf("Pagetables dump path: %s\n", pg_tables_dump_path);
		debug_printf("CR3 output path: %s\n", cr3_output_path);
		debug_printf("Initial cr3 values path: %s\n", initial_cr_values_path);
		}
	}

	if(world_rank == 0){
    	// debug_printf("%s pagetables\n", read_pgtables ? "Reading" : "Not readingn");
    	// debug_printf("Input is %s", input_is_file ? "file" : "pipe");
		// run_coordinator(world_size, read_pgtables, pg_tables_dump_path, initial_cr_values_path);
		while(true){}
	} else if(world_rank == 1) {
		run_input_reader(input_path, mapping_path, cr3_output_path, read_pgtables, input_is_file);
	}else {
		while(true){}
		// run_worker(world_size-2);
	}
	MPI_Finalize();
	return 0;
}
