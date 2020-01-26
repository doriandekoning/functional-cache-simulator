#include <stdint.h>
#include <mpi.h>
#include <stdlib.h>
#include <stdbool.h>

#include "filereader/mpireader.h"
#include "control_registers/control_registers.h"
#include "filereader/sharedmemreader.h"
#include "mappingreader/mappingreader.h"
#include "pagetable/pagetable.h"
#include "simulator/distributed/mpi_datatype.h"

#include "config.h"

struct WorkerBuffer {
    size_t cur_write_idx;
    size_t size;
    uint8_t cur_buffer_idx;
    cache_access *accesses_buf_0, *accesses_buf_1;
    MPI_Request cur_open_request;
};

int process_cache_access(struct WorkerBuffer* buffers, cache_access* access, int world_size) {
    //Determine which worker to send access to
    //TODO support sending to multiple workers
    int worker_idx = (access->address / CACHE_LINE_SIZE) % (world_size-2);
    struct WorkerBuffer* wb = &buffers[worker_idx];
    if(wb->cur_buffer_idx = 0 ){
        wb->accesses_buf_0[wb->cur_write_idx] = *access;
    }else{
        wb->accesses_buf_1[wb->cur_write_idx] = *access;
    }
    wb->cur_write_idx++;
    if(wb->cur_write_idx == wb->size) {
        if(wb->cur_open_request != NULL) {
            if(MPI_Wait(&wb->cur_open_request, MPI_STATUS_IGNORE)) {
                printf("Unable to wait!\n");
                return 1;
            }
        }
        if(wb->cur_buffer_idx = 0) {
            if(MPI_Isend(wb->accesses_buf_0, wb->size*sizeof(cache_access), MPI_CHAR, worker_idx, 2, MPI_COMM_WORLD, &wb->cur_open_request)) {
                printf("Unable to send!\n");
                return 1;
            }
            wb->cur_buffer_idx = 1;
        }else{
            if(MPI_Isend(wb->accesses_buf_1, wb->size*sizeof(cache_access), MPI_CHAR, worker_idx, 2, MPI_COMM_WORLD, &wb->cur_open_request)) {
                printf("Unable to send!\n");
                return 1;
            }
            wb->cur_buffer_idx = 0;
        }
        wb->cur_write_idx = 0;
    }
    return 0;
}


int run_coordinator(int mpi_world_size) {
    uint64_t mem_range_start = 0;
	// uint64_t mem_range_end = 0x23fffffffULL; 8g
	uint64_t mem_range_end = 1024*1024*1024; //1g
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
    bool cache_disable, is_user_page_access;
    ControlRegisterValues control_register_values;
    struct Memory* simulated_memory;

    struct WorkerBuffer* worker_buffers;

    amount_mem_accesses = 0;
    mpi_buffer = setup_mpi_buffer(SHARED_MEM_BUF_SIZE, mpi_world_size -1);
    simulated_memory = init_memory(NULL);
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

    control_register_values = init_control_register_values(AMOUNT_SIMULATED_PROCESSORS);
    worker_buffers = malloc(sizeof(struct WorkerBuffer) * (mpi_world_size-2));
    for(int i = 0; i < (mpi_world_size - 2); i++ ){
        worker_buffers[i].size = 1024;
        worker_buffers[i].cur_write_idx = 0;
        worker_buffers[i].accesses_buf_0 = malloc(sizeof(cache_access)*1024);
        worker_buffers[i].accesses_buf_1 = malloc(sizeof(cache_access)*1024);
        worker_buffers[i].cur_open_request = NULL;
        worker_buffers[i].cur_buffer_idx = 0;
    }

    uint64_t amount_cache_disable = 0;
    uint64_t amount_cache_enable = 0;

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
            if(amount_mem_accesses == 100000000){
                printf("100M\n");
                return 1;
            }
            if(mpi_buffer_get_memory_access(mpi_buffer, &tmp_access, next_event_id == trace_event_id_mapping->guest_mem_store_before_exec)) {
				printf("Can not read cache access\n");
				return 1;
			}
            tmp_access.tick = current_timestamp;
            cache_disable = false;

            #ifdef ADDRESS_TRANSLATION
            if(paging_enabled(control_register_values, tmp_access.cpu)){
                uint64_t physical_addr = 0;
                uint64_t cr3 = get_cr_value(control_register_values, tmp_access.cpu, 3);
                if( get_cr_value(control_register_values, tmp_access.cpu, 4) & (1U << 5)) {
        			physical_addr = vaddr_to_phys(simulated_memory, cr3 & ~0xFFFUL, tmp_access.address, false, &is_user_page_access, &cache_disable);
                }else{
                    printf("32bit?!\n");
                    return 1;
                }
                if(physical_addr == NOTFOUND_ADDRESS) {//} || physical_address != tmp_access->physaddress)  {
                    printf("Not able to translate virt:%lx\n", tmp_access.address);
		        	return 1;
                }
                tmp_access.address = physical_addr;
            }

            if(tmp_access.type == CACHE_EVENT_WRITE && !is_user_page_access
				&& mem_range_start <= tmp_access.address && tmp_access.address < mem_range_end ) {
				if(write_sim_memory(simulated_memory, tmp_access.address, tmp_access.size, &(tmp_access.data)) !=  tmp_access.size) {
					printf("Unable to write memory!\n");
					return 1;
				}
			}
            #endif

            if(!cache_disable && process_cache_access(worker_buffers, &tmp_access, mpi_world_size)) {
                printf("unable to process cache access!\n");
                return 1;
            }else if(cache_disable) {
                amount_cache_disable++;
                printf("Cache disabled%lu/%lu!\n", amount_cache_disable, amount_cache_enable);//Directly write to output
            }else{
                amount_cache_enable++;
            }
		} else if(next_event_id == trace_event_id_mapping->guest_update_cr) {
			if(mpi_buffer_get_cr_change(mpi_buffer, &tmp_cr_change)) {
				printf("Can not read cr change\n");
				return 1;
			}
            // tmp_cr_change->tick = current_timestamp;
            set_cr_value(control_register_values, tmp_cr_change.cpu, tmp_cr_change.register_number, tmp_cr_change.new_value);

		} else if(next_event_id == trace_event_id_mapping->guest_flush_tlb_invlpg) { //TODO rename trace_mapping to trace_event_mapping
            uint64_t addr;
			uint8_t cpu;
			if(mpi_buffer_get_invlpg(mpi_buffer, &addr, &cpu)){
				printf("Cannot read invlpg\n");
				return 1;
			}
            //Currently unused

		} else if(next_event_id == trace_event_id_mapping->guest_start_exec_tb) {
            if(mpi_buffer_get_tb_start_exec(mpi_buffer, &tmp_tb_start_exec)) {
				printf("Cannot read tb_start_exec\n");
				return 1;
			}
            int i = 0;
            while(tmp_tb_start_exec.size - (i*CACHE_LINE_SIZE) > -CACHE_LINE_SIZE) {
                tmp_access.address = tmp_tb_start_exec.pc + (i*CACHE_LINE_SIZE);
                tmp_access.tick = current_timestamp;
                tmp_access.type = CACHE_EVENT_INSTRUCTION_FETCH;
                tmp_access.cpu = tmp_tb_start_exec.cpu;
                if(process_cache_access(worker_buffers, &tmp_access, mpi_world_size)) {
                    printf("Cannot process access!\n");
                    return 1;
                }
                i++;
            }
            //TODO send to workers
		}else {
	        printf("Unknown eventid: %x\n", next_event_id);
			break;
		}
    }

    return 0;
}
