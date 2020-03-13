#include <mpi.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "config.h"
#include "coordinator.h"
#include "worker.h"
#include "simulator/distributed/input_reader.h"
#include "simulator/distributed/output.h"
// #include "simulator/distributed/mpi_datatype.h"

#include "cache/state.h"


int main(int argc, char** argv) {
	int provided, world_size, world_rank, name_len;//TODO rename better
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	char *mapping_path, *initial_cr_values_path, *cr_input_path, *input_path, *memrange_path, *output_path, *memdump_path;
	int amount_cpus = -1;

	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	if(world_size<= 2) {
		printf("World size needs to be at least 3\n");
		return -1;
	}
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Get_processor_name(processor_name, &name_len);


	if(argc < 8) {
		printf("At least 4 arguments need to be specified:\n");
		printf("Argument 1: trace event id mapping path!\n");
		printf("Argument 2: the initial cr values path!\n");
		printf("Argument 3: input path!\n");
		printf("Argument 4: memory ranges file!\n");
		printf("Argument 5: amount of simulated processors!\n");
		printf("Argument 6: output path!\n");
		printf("Argument 7: memdump basepath!\n");
		return 1;
	}
	mapping_path = argv[1];
	cr_input_path = argv[2];
	input_path = argv[3];
	memrange_path = argv[4];
	amount_cpus = atoi(argv[5]);
	output_path = argv[6];
	memdump_path = argv[7];
	printf("Simualting cache for %d cpus!\n", amount_cpus);
	if(amount_cpus <= 0) {
		printf("Invalid value for amount of cpus: %d\n", amount_cpus);
		return 1;
	}

	if(world_rank == (world_size - 2)){
		time_t start = time(NULL);
		if(run_coordinator(world_size, amount_cpus, memdump_path)) {
			return 1;
		}
		printf("Time in seconds:%lu\n", time(NULL) - start);
		return 1;
	} else if(world_rank == (world_size -1)) {
		time_t start = time(NULL);
		debug_printf("[INPUT_READER]Starting input reader with rank: %d\n", world_size -1);
		run_input_reader(input_path, mapping_path, memrange_path, cr_input_path, world_size, amount_cpus);
		printf("Time in seconds:%lu\n", time(NULL) - start);
		return 1;
	}else if( world_rank == (world_size - 3) ) {
		debug_printf("[OUPUT]Starting output writer with rank: %d\n", world_size -3);
		if( run_output(world_size, world_size - 3, world_size - 3, output_path)){
			return 1;
		}
	}else {
		if(run_worker(world_size, world_rank, amount_cpus)) {
			return 1;
		}
	}
	printf("Finished, rank:%d\n", world_rank);
	MPI_Finalize();
	return 0;
}
