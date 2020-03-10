#include <stdint.h>
#include <mpi.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "filereader/mpireader.h"
#include "control_registers/control_registers.h"
#include "filereader/sharedmemreader.h"
#include "mappingreader/mappingreader.h"
#include "pagetable/pagetable.h"
#include "simulator/distributed/mpi_datatype.h"
#include "filereader/mpireader.h"

#include "memory/memory_range.h"

#include "config.h"

struct WorkerBuffer {
    size_t cur_write_idx;
    size_t size;
    uint8_t cur_buffer_idx;
    cache_access *accesses_buf_0, *accesses_buf_1;
    MPI_Request cur_open_request;
};



struct MemoryRange* receive_memory_ranges_from_inputreader(int input_reader_rank) {
	MPI_Datatype memory_range_datatype;
	if(get_mpi_memoryrange_datatype(&memory_range_datatype)) {
		printf("Unable to setup MPI memoryrange datatype!\n");
		return NULL;
	}

    MPI_Status status;
    debug_printf("[COORDINATOR]waiting for probe from input reader with rank: %d\n", input_reader_rank);
    if(MPI_Probe(input_reader_rank, MPI_TAG_MEMRANGE, MPI_COMM_WORLD, &status) != MPI_SUCCESS)  {
        printf("Unable to probe!\n");
        return NULL;
    }
    printf("[COORDINATOR]received probe\n");
    int count;
    if(MPI_Get_count(&status, memory_range_datatype, &count) != MPI_SUCCESS ) {
        printf("Unable to get count of memory ranges!\n");
        return NULL;
    }
    struct MemoryRange* buf = malloc(count * sizeof(struct MemoryRange));
    if(MPI_Recv(buf, count, memory_range_datatype, input_reader_rank, MPI_TAG_MEMRANGE, MPI_COMM_WORLD, &status) != MPI_SUCCESS) {
        printf("Unable to receive memory range from inputreader!\n");
        return NULL;
    }
    buf[count-1].next = NULL;
    for(int i = 0; i < (count-1); i++ ) {
        buf[i].next = &(buf[i+1]);
    }
    return &(buf[0]);
}

int process_cache_access(struct WorkerBuffer* buffers, cache_access* access, int world_size) {


    //Determine which worker to send access to
    //TODO support sending to multiple workers
    int worker_idx = (access->address / CACHE_LINE_SIZE) % (world_size-3);
    struct WorkerBuffer* wb = &buffers[worker_idx];
    if(wb->cur_buffer_idx == 0 ){
        wb->accesses_buf_0[wb->cur_write_idx] = *access;
    }else{
        wb->accesses_buf_1[wb->cur_write_idx] = *access;
        }
    wb->cur_write_idx++;
    if(wb->cur_write_idx == wb->size) {
        debug_printf("[COORDINATOR]sending buffer: %d\n", wb->cur_buffer_idx);
        if(wb->cur_open_request != NULL) {
            if(MPI_Wait(&wb->cur_open_request, MPI_STATUS_IGNORE)) {
                printf("Unable to wait!\n");
                return 1;
            }
        }
        if(wb->cur_buffer_idx == 0) {

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
        debug_printf("[COORDINATOR]send buffer: %d\n",  wb->cur_buffer_idx);
    }
    return 0;
}


int run_coordinator(int mpi_world_size, int amount_cpus, char* memory_range_base_path) {
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
    bool cache_disable, is_user_page_access;
    ControlRegisterValues control_register_values;
    struct Memory* simulated_memories;

    struct WorkerBuffer* worker_buffers;
    struct MemoryRange* memranges;

        #ifdef GDB_DEBUG
	volatile int i = 0;
	char hostname[256];
    gethostname(hostname, sizeof(hostname));
    printf("PID %d on %s ready for attach\n", getpid(), hostname);
    fflush(stdout);
	while(0 == i){
		printf("Sleeping!%d\n",i);
		sleep(5);
	}
	printf("Continueueing!\n");
	#endif

    mpi_buffer = setup_mpi_buffer(SHARED_MEM_BUF_SIZE, mpi_world_size -1);
    current_timestamp = 0;
    trace_event_id_mapping = malloc(sizeof(struct EventIDMapping));
    if(get_mpi_eventid_mapping_datatype(&mpi_mapping_datatype)) {
        printf("Unable to setup MPI datatype!\n");
        return 1;
    }
    debug_print("[COORDINATOR]\twaiting to receive memory ranges\n");
    memranges = receive_memory_ranges_from_inputreader(mpi_world_size - 1);
    if(memranges == NULL){
        printf("Unable to receive memory ranges from input reader!\n");
        return 1;
    }
    debug_print("[COORDINATOR]\tReceived memory ranges from input reader:\n");
    struct MemoryRange* cur = memranges;
    int amount_memories = 0;
    while(cur != NULL) {
        printf("[%016lx-%016lx]\n", cur->start_addr, cur->end_addr);
        amount_memories++;
        cur = cur->next;
    }

    simulated_memories = malloc(amount_memories * sizeof(struct Memory));
    if(simulated_memories == NULL) {printf("Unable to setup memories!\n"); return 1;}

    printf("Setting up %d memories!\n", amount_memories);
    cur = memranges;
    for(int i = 0; i < amount_memories; i++) {
        printf("%d\n", i);
        char* bfile_path = calloc(1, strlen(memory_range_base_path) + 17);
        sprintf(bfile_path, "%s-%lx", memory_range_base_path, cur->start_addr);
        FILE* bfile = fopen(bfile_path, "r");
        if(bfile == NULL) {
            printf("Unable to open bfile:%s\n", bfile_path);
            exit(1);
        }
        free(bfile_path);
        struct Memory* mem = init_memory(bfile, cur->start_addr, cur->end_addr);
        if(mem == NULL) {
            printf("Unabel to setup memory!\n");
            return 1;
        }
        simulated_memories[i] = *mem;
        if(i >= 10){
            printf("Maximum amount of memory ranges exceeded!\n");
            exit(1);
        }
        cur = cur->next;
    }

    debug_print("[COORDINATOR]\twaiting to receive id mapping\n");
    if(MPI_Recv(trace_event_id_mapping, 1, mpi_mapping_datatype, mpi_world_size-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE) != MPI_SUCCESS ) {
        printf("Unable to receive mapping!\n");
        return 1;
    }
    debug_print("[COORDINATOR]\tReceived trace event id mapping!\n");
    print_mapping(trace_event_id_mapping);

    control_register_values = init_control_register_values(amount_cpus);

    if(MPI_Recv(control_register_values, AMOUNT_CONTROL_REGISTERS*amount_cpus, MPI_UINT64_T, mpi_world_size-1, MPI_TAG_CONTROLREGISTERVALUES, MPI_COMM_WORLD, MPI_STATUS_IGNORE) != MPI_SUCCESS) {
        printf("Unable to receive initial control register values!\n");
        return 1;
    }

    worker_buffers = malloc(sizeof(struct WorkerBuffer) * (mpi_world_size-3));
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
    uint64_t total_amount_events_proccessed = 0;
    debug_print("[COORDINATOR]\t starting");
    while(1) {
        total_amount_events_proccessed++;
        if(total_amount_events_proccessed % 10000000 == 0) {
            printf("%lu Mil events processed!\n", total_amount_events_proccessed / 1000000);
        }
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
        			physical_addr = vaddr_to_phys(amount_memories, simulated_memories, cr3 & ~0xFFFUL, tmp_access.address, &is_user_page_access, &cache_disable);
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
            if(tmp_access.type == CACHE_EVENT_WRITE && !is_user_page_access ) { //TODO make this memory range check
				write_sim_memory(amount_memories, simulated_memories, tmp_access.address, tmp_access.size, &(tmp_access.data));
			}
            #endif
            if(!cache_disable && process_cache_access(worker_buffers, &tmp_access, mpi_world_size)) {
                printf("unable to process cache access!\n");
                return 1;
            }else if(cache_disable) {
                amount_cache_disable++;
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
		}else {
	        printf("Unknown eventid: %x\n", next_event_id);
			break;
		}
    }

    return 0;
}
