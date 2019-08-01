#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "messages/packet.pb-c.h"
#include "varint/varint.h"

clock_t start, end;
clock_t start_read;
clock_t start_decode;

#define LIMIT 50000000


int read_own_buffer(FILE* input) {
	int amount_packages_read = 0;
    start_read = clock();
    uint8_t* bytes = malloc(LIMIT * 20);
    if (fread(bytes, 20, LIMIT, input) != LIMIT) {
        printf("Could not read complete file\n");
        return -1;
    }
    printf("Time spend on reading:%lu\n", clock() - start_read);
    uint32_t offset = 0;
    start_decode = clock();
    do{
        unsigned char n;
		long long msg_len = varint_decode((char*)&bytes[offset], 1, &n);
        offset+=n;
        Messages__Packet* packet = messages__packet__unpack(NULL, msg_len, &bytes[offset]);
        offset+=msg_len;
        if(packet == NULL) {
			printf("The %dth packet is NULL\n", amount_packages_read);
            continue;
        }
        amount_packages_read++;
        protobuf_c_message_free_unpacked((ProtobufCMessage *)packet, NULL); //TODO cast to prevent compiler warning

    } while(amount_packages_read < LIMIT && offset < (LIMIT*20));
    return 0;
}

int read_fread(FILE* input) {


    int success = setvbuf(input, NULL, _IOFBF, 1024*1024);
    if(success != 0 ){
        printf("Cannot allocate IO buffer!\n");
    }

    unsigned char buf[4048];
    int amount_packages_read = 0;
    int inamount = 0;
     while(true){
		char varint[1];
		fread(varint, 1, 1, input);
		long long msg_len = varint_decode(varint, 1, buf);
	    inamount = fread(buf, 1, msg_len, input); //TODO read packets
        if(inamount < msg_len) {
            printf("Found end of file\n");
            break;
        }

 		if(amount_packages_read > LIMIT) {
			printf("Simulation limit reached%d\n", amount_packages_read);
			break;
		}
        Messages__Packet* packet = messages__packet__unpack(NULL, msg_len, buf);
        if(packet == NULL) {
			printf("The %dth packet is NULL\n", amount_packages_read);
            continue;
        }
        amount_packages_read++;

         protobuf_c_message_free_unpacked((ProtobufCMessage *)packet, NULL); //TODO cast to prevent compiler warning

     }
     return 0;
}


FILE* setup_infile(char* loc) {
    FILE *infile = malloc(sizeof(infile));
    unsigned char buf[4048];

    infile = fopen(loc, "r+");
    if(!infile) {
        printf("Error occured opening file\n");
        exit(-1);//TODO add status exit codes
    }

    int success = setvbuf(infile, NULL, _IOFBF, (LIMIT*20));
    if(success != 0 ){
        printf("Cannot allocate IO buffer!\n");
    }

    //Read file header (should say "gem5")
    char fileHeader[4];
    if( fread(fileHeader, 1, 4, infile) != 4){
        printf("Could not read file header");
        return NULL;
    }

	//Read length of header packet
	char varint[1];
	if(fread(varint, 1, 1, infile) != 1){
        printf("Could not read header packet\n");
        return NULL;
    }
	long long headerlength = varint_decode(varint, 1, NULL);
	fseek(infile, headerlength, SEEK_CUR);

    return infile;
}


int main(int argc, char** argv) {
    if(argc <= 1) {
        printf("First argument should be input file location!\n");
        return -1;
    }




    clock_t start = clock();
    read_own_buffer(setup_infile(argv[1]));
    printf("Time spend when reading complete file at once:\t%f\n", (double)(clock() - start)/CLOCKS_PER_SEC);
    start = clock();
    read_fread(setup_infile(argv[1]));
    printf("Time spend when using fread:\t%f\n", (double)(clock() - start)/CLOCKS_PER_SEC);



	return 0;
}

