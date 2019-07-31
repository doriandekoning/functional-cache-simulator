#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "messages/packet.pb-c.h"
#include "varint/varint.h"

clock_t start, end;

#define LIMIT 50000000000


int main(int argc, char** argv) {
    if(argc <= 1) {
        printf("First argument should be input file location!\n");
        return -1;
    }
    FILE *infile;
    unsigned char buf[4048];
    int inamount = 0;

    infile = fopen(argv[1], "r+");
    if(!infile) {
        printf("Error occured opening file\n");
        exit(-1);//TODO add status exit codes
    }

    int success = setvbuf(infile, NULL, _IOFBF, 1024*1024);
    if(success != 0 ){
        printf("Cannot allocate IO buffer!\n");
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
    start = clock();
    while(true){
		char varint[1];
		fread(varint, 1, 1, infile);
		long long msg_len = varint_decode(varint, 1, buf);
	    inamount = fread(buf, 1, msg_len, infile); //TODO read packets
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
    end = clock();
    printf("Time used by coordinator:\t%f\n", (double)(end-start)/CLOCKS_PER_SEC);
    printf("Total messages send:\t\t%d\n", amount_packages_read);
	return 0;
}


