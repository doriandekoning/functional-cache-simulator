#include <stdio.h>
#include <stdlib.h>

#include "pipereader.h"

int main(int argc, char** argv){
	if(argc<3){
		printf("First argument is the input file or pipe, second argument should be the path to the output file\n");
		return 1;
	}
	FILE *f;
	f = fopen(argv[1], "r+");
	if(!f){
		printf("Could not open input file\n");
		return 2;
	}

	if(read_header(f) != 0){
		printf("Unable to read header\n");
		return 3;
	}

	FILE *out = fopen(argv[2], "w");
	if(!out) {
		printf("Could not open output file!\n");
		return 4;
	}
	printf("Read header!\n");
	uint64_t* crs = malloc(500 * sizeof(uint64_t));
	int totalcrs = 0; 
	char* buffer = malloc(1000* sizeof(char));
	cr_change* tmp_cr_change = malloc(sizeof(cr_change));
	while(true) {
		int next_eventid = get_next_event_id(f);
		if(next_eventid == (uint8_t)-1) {
			printf("Could not get next eventid!\n");
			break;
		}
		if(next_eventid == 70) {
			if(get_cr_change(f, tmp_cr_change)) {
				printf("Could not read cr change!\n");
				break;
			}
			if(tmp_cr_change->register_number != 3) {
				continue;
			}
			uint64_t new_cr = tmp_cr_change->new_value;
			bool found = false;
			for(int i = 0; i < totalcrs; i++) {
				if(crs[i] == new_cr){
					found = true;
					break;
				}
			}
			if(!found){
				crs[totalcrs] = new_cr;
				sprintf(buffer, "%lu\n", new_cr);
				if(fputs(buffer, out) == EOF) {
					printf("Could not write!\n");
					perror("Error occured\n");
					return 50;
				}
				totalcrs++;
				if(totalcrs >= 500){
					printf("Too many crs\n");
					return 1;
				}
			}
		}else{
			printf("Unknown event!%lu\n", next_eventid);
			break;
		}
	}
	if(fclose(out) != 0 ) {
		perror("Could not close file");
	}
	free(tmp_cr_change);
	free(crs);
	free(buffer);


}
