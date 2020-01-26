#include <stdint.h>
#include <mpi.h>
#include <stdlib.h>
#include <stdbool.h>

#include "filereader/mpireader.h"
#include "filereader/sharedmemreader.h"
#include "mappingreader/mappingreader.h"
#include "simulator/distributed/mpi_datatype.h"

#include "config.h"

int run_coordinator(int mpi_world_size) {
    struct mpi_buffer* mpi_buffer;
    uint8_t next_event_id;
    uint64_t current_timestamp;
    uint64_t delta_t;
    bool negative_delta_t;
    MPI_Datatype mpi_mapping_datatype;
    struct EventIDMapping* trace_event_id_mapping;
    cache_access tmp_access;
	cr_change tmp_cr_change;
	tb_start_exec tmp_tb_start_exec;
    uint64_t amount_mem_accesses;

    amount_mem_accesses = 0;
    mpi_buffer = setup_mpi_buffer(SHARED_MEM_BUF_SIZE, mpi_world_size -1);
    current_timestamp = 0;
    trace_event_id_mapping = malloc(sizeof(struct EventIDMapping));
    if(get_mpi_eventid_mapping_datatype(&mpi_mapping_datatype)) {
        printf("Unable to setup MPI datatype!\n");
        return 1;
    }

    if(MPI_Recv(trace_event_id_mapping, 1, mpi_mapping_datatype, mpi_world_size-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE) != MPI_SUCCESS ) { //TODO not ignore status
        printf("Unable to receive mapping!\n");
        return 1;
    }
    printf("Received mapping!\n");
    print_mapping(trace_event_id_mapping);

    while(1) {
        if(mpi_buffer_get_next_event_id(mpi_buffer, &delta_t, &negative_delta_t, &next_event_id)) {
            printf("Unable to read next event id!\n");
            return 1;
        }
        if(negative_delta_t) {
            current_timestamp -= delta_t;
        }else{
            current_timestamp += delta_t;
        }

        if(next_event_id == trace_event_id_mapping->guest_mem_load_before_exec || next_event_id == trace_event_id_mapping->guest_mem_store_before_exec ) {
            amount_mem_accesses++;
            if(amount_mem_accesses && amount_mem_accesses % 10000000 == 0){
                printf("amount_mem_accesses:%lu Million\n", amount_mem_accesses / 1000000);
            }
            if(mpi_buffer_get_memory_access(mpi_buffer, &tmp_access, next_event_id == trace_event_id_mapping->guest_mem_store_before_exec)) {
				printf("Can not read cache access\n");
				break;
			}
		} else if(next_event_id == trace_event_id_mapping->guest_update_cr) {
			if(mpi_buffer_get_cr_change(mpi_buffer, &tmp_cr_change)) {
				printf("Can not read cr change\n");
				break;
			}
		} else if(next_event_id == trace_event_id_mapping->guest_flush_tlb_invlpg) { //TODO rename trace_mapping to trace_event_mapping
            uint64_t addr;
			uint8_t cpu;
			if(mpi_buffer_get_invlpg(mpi_buffer, &addr, &cpu)){
				printf("Cannot read invlpg\n");
				break;
			}
		} else if(next_event_id == trace_event_id_mapping->guest_start_exec_tb) {
            if(mpi_buffer_get_tb_start_exec(mpi_buffer, &tmp_tb_start_exec)) {
				printf("Cannot read tb_start_exec\n");
				break;
			}
		}else {
	        printf("Unknown eventid: %x\n", next_event_id);
			break;
		}
    }

}
