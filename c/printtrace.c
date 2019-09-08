#include "pipereader.h"
#include "cachestate.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	if(argc < 2) {
		printf("Atleast one argument must be provieded!\n");
		return 10;
	}

	FILE *in;
	unsigned char buf[4048];
	in = fopen(argv[1], "r+");
	if(!in){
		printf("Could not open file\n");
		return 1;
	}

	if(read_header(in) != 0) {
		printf("Unable to read header!\n");
		return 2;
	}

	cache_access* tmp_access = malloc(sizeof(cache_access));
	cr3_change* tmp_cr3_change = malloc(sizeof(cr3_change));
	while(true) {
		uint8_t next_event_id = get_next_event_id(in);
		if(next_event_id < 0) {
			printf("Could not read eventid!\n");
			break;
		}
		if(next_event_id == 67) {
			if(get_cache_access(in, tmp_access)) {
				printf("Can not read cache access\n");
				break;
			}
			printf("Tick:%lu\n", tmp_access->tick);
		} else if(next_event_id == 68) {
			if(get_cr3_change(in, tmp_cr3_change)) {
				printf("Can not read cr3\n");
			}
		}else {
			printf("Unknown eventid: %x\n", next_event_id);
		//	break;
		}
	
	}
}


