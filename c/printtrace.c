#include "pipereader.h"
#include "cachestate.h"
#include <stdio.h>
#include<stdlib.h>

#define PG_MASK (1 << 31)
#define PE_MASK (1 << 0)
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
	uint64_t kernel_reads_64 = 0;
	uint64_t kernel_reads_other = 0;
	uint64_t kernel_writes = 0;
	uint64_t user_access = 0;
	uint64_t write_phys = 0;
	uint64_t write_virt = 0;
	uint8_t paging_enabled = 0;
	cache_access* tmp_access = malloc(sizeof(cache_access));
	cr_change* tmp_cr_change = malloc(sizeof(cr_change));
	while(true) {
		uint8_t next_event_id = get_next_event_id(in);
		if(next_event_id == (uint8_t)-1) {
			printf("Could not read eventid!\n");
			break;
		}
		if(next_event_id == 67 || next_event_id == 69) {
			if(get_cache_access(in, tmp_access)) {
				printf("Can not read cache access\n");
				break;
			}
			if(tmp_access->type == CACHE_WRITE && paging_enabled == 2) {
				write_virt++;
			}else if(tmp_access->type == CACHE_WRITE){
				write_phys++;
			}
			if(tmp_access->address & (1L << 63)){

				if(tmp_access->type == CACHE_READ && tmp_access->size == 8){
					kernel_reads_64++;
				}else if(tmp_access->type == CACHE_READ){
					kernel_reads_other++;
				}else{
					kernel_writes++;
				}
			}else{
				user_access++;
			}
		} else if(next_event_id == 70) {
			if(get_cr_change(in, tmp_cr_change)) {
				printf("Can not read cr change\n");
			}
			uint64_t newval = tmp_cr_change->new_value;
			if(tmp_cr_change->register_number==0){
				if(paging_enabled == 2 && (!(newval & PE_MASK) || !(newval & PG_MASK))){
					printf("Paging disabled!\n");
					paging_enabled=1;
				}else if(paging_enabled <= 1 && (newval & PE_MASK) && (newval & PG_MASK)){
					printf("Paging enabled!\n");
					paging_enabled=2;
				}
			}else if(tmp_cr_change->register_number == 3) {
				printf("CR3:%lu\n", tmp_cr_change->new_value);
			}
		}else {
			printf("Unknown eventid: %x\n", next_event_id);
			break;
		}
	
	}
	printf("Kernel reads 64:%lu\n", kernel_reads_64);
	printf("Kernel reads other:%lu\n", kernel_reads_other);
	printf("Physwrite \t%lu\n", write_phys);
	printf("Virtwrite \t%lu\n", write_virt);
	printf("Kernel writes\t%lu\n", kernel_writes);
	printf("User accesses:\t%lu\n", user_access);
}


