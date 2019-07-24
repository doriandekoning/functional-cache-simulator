#include "mpi.h"
#include "messages/packet.pb-c.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "cachestate.h"
#include <stdlib.h>
#include "varint/varint.h"


#define SIMULATION_LIMIT 10000000



MPI_Datatype mpi_access_type;


typedef struct access_s {
	uint64_t address;
	uint64_t tick;
	uint64_t cpu;
	bool write;
} access;

int amount_writebacks = 0;
int amount_misses = 0;

CacheSetState check_eviction(CacheSetState state) {
	int length = list_length(state);
	if(length > ASSOCIATIVITY) {
		if(state->prev != NULL){
			printf("ERROR prev is not NULL");
		}
		struct CacheEntry* lru = get_head(state);
		if(lru->state == STATE_MODIFIED) { 
			amount_writebacks++;
		}
		struct statechange change;
		change.new_state = STATE_INVALID;
		return apply_state_change(state, lru, change, 0);

	}
	return state;
}

int worker(int amount_workers) {
	printf("Allocating a state array of size:%d * %lu\n", AMOUNT_SIMULATED_PROCESSORS * (CACHE_SIZE/amount_workers), sizeof(struct CacheEntry*));
	CacheState states = calloc(AMOUNT_SIMULATED_PROCESSORS * (CACHE_SIZE/amount_workers), sizeof(struct CacheEntry*)); 
	access msg;
	for(int totalPackets = 0; totalPackets < SIMULATION_LIMIT / amount_workers; totalPackets ++ ){
		MPI_Status status;//. = malloc(sizeof(MPI_Status));
		int result;
		MPI_Probe(0, 0, MPI_COMM_WORLD, &status);
		int count;
		MPI_Get_elements(&status, mpi_access_type, &count);
		if (count == 0) {
			printf("Finished\n");
			break;
		}
		MPI_Recv(&msg, 1, mpi_access_type, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		uint64_t cache_line = msg.address / CACHE_LINE_SIZE;
		int cache_set_number = get_cache_set_number(cache_line);
		CacheSetState set_state = get_cache_set_state(states, cache_line, msg.cpu);
		// printf("HIER5%d\n", msg.address);

		CacheEntryState entry_state = get_cache_entry_state(set_state, cache_line);
		int cur_state = STATE_INVALID;
		if(entry_state != NULL) {
			cur_state = entry_state->state;
		}


		struct statechange state_change = get_msi_state_change(cur_state, msg.write);
		CacheSetState new = apply_state_change(set_state, entry_state, state_change, cache_line);
		new = check_eviction(new);
		set_cache_set_state(states, new, cache_line, msg.cpu);

		// Update state in other cpus
		bool found_in_other_cpu = false;
		for(int i = 0; i < AMOUNT_SIMULATED_PROCESSORS; i++) {
			if(i != msg.cpu) {
				CacheSetState cache_set_state = get_cache_set_state(states, cache_line, i);
				CacheEntryState cache_entry_state = get_cache_entry_state(cache_set_state, cache_line);
				if(cache_entry_state != NULL && cache_entry_state->state != STATE_INVALID) {//TODO check if we need to do this with invalid
					found_in_other_cpu = true;
					bool flush = false;
					struct statechange new_state = get_msi_state_change_by_bus_request(cache_entry_state->state, state_change.bus_request);
					CacheSetState new = apply_state_change(cache_set_state, cache_entry_state, state_change, cache_line);
					new = check_eviction(cache_set_state);
					set_cache_set_state(states, new, cache_line, msg.cpu);

				}

			}
		}
		if(state_change.bus_request == BUS_REQUEST_READ && !found_in_other_cpu) {
			// printf("Miss%llu\n", msg.address);
			amount_misses++;
		}


	}

	printf("Amount writebacks:%d\n", amount_writebacks);
	printf("Amount misses:%d\n", amount_misses);


	return 0;
}


int coordinator(int world_size) {

    FILE *infile;
    uint8_t buf[4048];
    int inamount = 0;

    infile = fopen("input.trace", "r+");
    if(!infile) {
        printf("Error occured opening file\n");
        return -1;
    }
    //Read file header (should say "gem5")
    char fileHeader[4];
    inamount = fread(fileHeader, 1, 4, infile);
    if(inamount != 4){
        printf("Could not read file header");
        return -1;
    }

	//Read length of header packet
	char varint[1];
	fread(varint, 1, 1, infile);
	long long headerlength = varint_decode(varint, 1, NULL);
	fseek(infile, headerlength, SEEK_CUR);
	int amount_packages_read = 0;
	//Open requests
	int max_open_requests = 1;
	MPI_Request* open_requests = calloc(max_open_requests, sizeof(access));
	access* open_messages = calloc(max_open_requests, sizeof(access));
	int open_request_counter = 0;
	access* msg_ptr;
	MPI_Request* request_ptr;
    while(true){

		char varint[1];
		fread(varint, 1, 1, infile);
		long long msg_len = varint_decode(varint, 1, NULL);
	    inamount = fread(buf, 1, msg_len, infile); //TODO read packets
        if(inamount < msg_len) {
            printf("Error occured while reading, only read: %d bytes", inamount);
            return -1;
        }

		amount_packages_read++;

		if(amount_packages_read > SIMULATION_LIMIT) { //TODO make limit configurable
			printf("Simulation limit reached%d\n", amount_packages_read);
			// MPI_Waitall(max_open_requests, open_requests, MPI_STATUS_IGNORE);
			break;
		}
	  MPI_Status status;    

        Messages__Packet* packet;
        packet = messages__packet__unpack(NULL, msg_len, buf);

		MPI_Request* request_ptr;
        if(packet != NULL) {
			if(open_request_counter < max_open_requests){ 
				request_ptr = open_requests + (sizeof(MPI_Request) * open_request_counter);
				printf("%d, %p\n", open_request_counter, request_ptr);
				msg_ptr = open_messages + (sizeof(access) * open_request_counter);
				open_request_counter++;
			} else{
				int finished;
				MPI_Waitany(max_open_requests, open_requests, &finished, &status);//TODO check status
				request_ptr = open_requests + (sizeof(MPI_Request) * finished);
				msg_ptr = open_messages + (sizeof(access) * finished);
			}
			msg_ptr->address = packet->addr; //TODO map virtual to physical address
			msg_ptr->tick = packet->tick;
			msg_ptr->cpu = packet->cpuid;
			msg_ptr->write = packet->cmd == 1;

			int cacheSetNumber = (packet->addr >> 6) % (2 << 13);
			int receiver = cacheSetNumber%(world_size-1) + 1; // Add 1 to offset the rank of the coordinator
			//TODO check if grouping these messages is more efficient
			MPI_Isend(msg_ptr, 1, mpi_access_type, receiver, 0, MPI_COMM_WORLD, request_ptr);//TODO check ret

        }else{
			printf("The %dth packet is NULL\n", amount_packages_read);
		}
		protobuf_c_message_free_unpacked((ProtobufCMessage *)packet, NULL); //TODO cast to prevent compiler warning
    }
	for(int i = 1; i < world_size; i++) {
		printf("Sending\n");
		MPI_Send(NULL, 0, mpi_access_type, i, 0, MPI_COMM_WORLD);//TODO check ret
	}

	return 0;
}





int main(int argc, char** argv) {

	MPI_Init(&argc,&argv);

	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	if(world_size<= 1) {
		printf("World size needs to be at least 2\n");
		return -1;
	}

	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	int name_len;
	MPI_Get_processor_name(processor_name, &name_len);
	//Setup mpi type for struct access
	const int nitems = 4;
	int blocklengths[4] = {1,1,1,1};
	MPI_Datatype types[4] = {MPI_UINT64_T, MPI_UINT64_T, MPI_UINT64_T, MPI_C_BOOL};
	MPI_Aint offsets[4];
	offsets[0] = offsetof(access, address);
	offsets[1] = offsetof(access, tick);
	offsets[2] = offsetof(access, cpu);
	offsets[3] = offsetof(access, write);
	MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_access_type);
	MPI_Type_commit(&mpi_access_type);

	if(world_rank == 0){
		coordinator(world_size);
	}else {
		worker(world_size-1);
	}
	MPI_Finalize();
	return 0;
}

