#include "coordinator.h"
#include "config.h"
#include <mpi.h>


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "messages/packet.pb-c.h"
#include "varint/varint.h"
#include "cachestate.h"
#include "mpi_datatype.h"




int* stored_access_counts;
access* stored_accesses;

//TODO rank == (worldsize-1) is master

bool store_msg(int worker, Messages__Packet* packet) {
    int* index = &stored_access_counts[worker];
    access* access_ptr = &stored_accesses[ (((worker-1)*MESSAGE_BATCH_SIZE) + *index )];
    access_ptr->address = packet->addr; //TODO map virtual to physical address
	access_ptr->tick = packet->tick;
	access_ptr->cpu = packet->cpuid;
    access_ptr->write = packet->cmd == 1;
    (*index)++;
    return *index >= MESSAGE_BATCH_SIZE;
}

int send_accesses(int worker, MPI_Datatype mpi_access_type) {
    int success = MPI_Send(&stored_accesses[worker-1], MESSAGE_BATCH_SIZE, mpi_access_type, worker, 0, MPI_COMM_WORLD);//TODO
    stored_access_counts[worker-1] = 0; //This cannot be done for nonblocking sends
    if(success != MPI_SUCCESS) {
        printf("Unable to send accesses to worker: %d\n", success);
        return 1;
    } //TODO make ISend
    return 0;
}



int run_coordinator(int world_size) {
    FILE *infile;
    uint8_t buf[4048];
    int inamount = 0;

    MPI_Datatype mpi_access_type;
    int err = get_mpi_access_datatype(&mpi_access_type);
    if(err != 0){
        printf("Unable to create datatype, erorr:%d\n", err);
        return err;
    }
    //Setup access
    stored_access_counts = calloc(world_size-1, sizeof(int));
    stored_accesses = calloc((world_size-1) * MESSAGE_BATCH_SIZE, sizeof(access));


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
	int max_open_requests = 100;
	MPI_Request* open_requests = calloc(max_open_requests, sizeof(MPI_Request));
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
			printf("Waited for all requests to be finsihed\n");
			break;
		}
	 	MPI_Status status;

        Messages__Packet* packet;
        packet = messages__packet__unpack(NULL, msg_len, buf);

		// MPI_Request* request_ptr;
        if(packet != NULL) {
            int cacheSetNumber = (packet->addr >> 6) % (2 << 13);
            int worker = cacheSetNumber%(world_size-1) + 1; // Add 1 to offset the rank of the coordinator
            bool shouldSend = store_msg(worker, packet);
            if(shouldSend) {
                send_accesses(worker, mpi_access_type);
            }
			// if(open_request_counter < max_open_requests){
			// 	request_ptr = &open_requests[open_request_counter];
			// 	msg_ptr = &stored_accesses[worker];
			// 	open_request_counter++;
			// } else{
			// 	int finished_idx;
			// 	// MPI_Waitany(max_open_requests, open_requests, &finished_idx, &status);//TODO check status
			// 	request_ptr = &open_requests[finished_idx];
			// 	msg_ptr = &open_messages[finished_idx];
			// }





        }else{
			printf("The %dth packet is NULL\n", amount_packages_read);
		}
		protobuf_c_message_free_unpacked((ProtobufCMessage *)packet, NULL); //TODO cast to prevent compiler warning
    }

    //TODO final batch (send < MESSAGE_BATCH_SIZE (same tag) (0 if len of last batch was exactly == MESSAGE_BATCH_SIZE))
	for(int i = 1; i < world_size; i++) {
		printf("Sending\n");
		MPI_Send(NULL, 0, mpi_access_type, i, 0, MPI_COMM_WORLD);//TODO check ret
	}

	return 0;
}


