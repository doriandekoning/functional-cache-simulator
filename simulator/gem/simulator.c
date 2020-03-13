#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <time.h>

#include "cache/state.h"
#include "cache/hierarchy.h"
#include "cache/lru.h"
#include "cache/msi.h"
#include "cache/coherency_protocol.h"
#include "filereader/gemfilereader.h"

FILE* out;


struct __attribute__((__packed__)) OutputStruct{
	uint8_t type;
	int64_t timestamp;
	uint64_t address;
	uint8_t cpu;

};

uint64_t i = 0;
uint64_t mpki = 0;
uint64_t mpki_fetch = 0;
uint64_t mpki_write = 0;
uint64_t mpki_read = 0;
uint64_t write_miss_count = 0;
uint64_t read_miss_count = 0;
bool fffetch = 0;

void cache_miss_func(bool write, uint64_t timestamp, uint64_t address) {
	(void)write;
	(void)timestamp;
	(void)address;
	mpki++;
	if(write) {
		write_miss_count++;
	}else{
		read_miss_count++;
	}
	if(fffetch) {
		mpki_fetch++;
	}else{
		if(write) {
			mpki_write++;
		}else{
			mpki_read++;
		}
	}
	// struct OutputStruct outstruct = {
	// 	.timestamp = timestamp,
	// 	.address = address,
	// 	.type = write,
	// 	.cpu = 0,
	// };
	// fwrite(&outstruct, 1, sizeof(struct OutputStruct), out);
}

struct CacheHierarchy* setup_cache(int amount_cpus) {
	struct CacheHierarchy* hierarchy = init_cache_hierarchy(amount_cpus);
	struct CacheLevel* l1 = init_cache_level(amount_cpus, true);
	for(int i = 0; i < amount_cpus; i++) {
		struct CacheState* dcache = setup_cachestate(NULL, true, 32*1024, CACHE_LINE_SIZE, 8, &find_line_to_evict_lru, &msi_coherency_protocol, NULL);
		struct CacheState* icache = setup_cachestate(NULL, true, 32*1024, CACHE_LINE_SIZE, 8, &find_line_to_evict_lru, &msi_coherency_protocol, NULL);
		add_caches_to_level(l1, dcache, icache);
	}
	if(add_level(hierarchy, l1) != 0 ){
		exit(1);
	}
	struct CacheLevel* l2 = init_cache_level(amount_cpus, false);
	struct CacheState* state = setup_cachestate(NULL, true, 256*1024, CACHE_LINE_SIZE, 4, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
	add_caches_to_level(l2, state, NULL);
	if(add_level(hierarchy, l2) != 0 ){
		exit(1);
	}
	return hierarchy;
}


int main(int argc, char **argv) {
	printf("Cache simulator started!\n");
	struct CacheHierarchy* cache = setup_cache(4);

	if(argc < 2) {
		printf("Atleast 2 arguments, input path, output path\n");
		return 1;
	}
	char* input_path = argv[1];
	printf("Input path:%s\n", input_path);
	char* output_path = argv[2];
	printf("Ouptut path: %s\n", output_path);

	uint64_t read_count = 0;
	uint64_t write_count = 0;
	FILE *in = fopen(input_path, "rb");
	if(in == NULL) {
		printf("Could not open input file!\n");
		return 1;
	}

	out = fopen(output_path, "wb");
	if(out == NULL) {
		printf("Could not open output file: %s!\n", argv[3]);
		return 1;
	}
/*	uint64_t startoffset = 3000000000;
	i = startoffset;
	fseek(in, 14UL * startoffset, SEEK_SET);*/

	cache_access* tmp_access = malloc(sizeof(cache_access));
	printf("mpki,custom,custom_fetch,mpki_read,mpki_write\n");
	while(true) {
		i++;
		if(file_get_gem_memory_access(in, tmp_access)) {
			printf("Can not read cache access\n");
			break;
		}
		fffetch = (tmp_access->type == CACHE_EVENT_INSTRUCTION_FETCH);
		if(tmp_access->type == CACHE_EVENT_WRITE) {
			write_count++;
		}else{
			read_count++;
		}
		access_cache_in_hierarchy(cache, tmp_access->cpu, tmp_access->address, tmp_access->tick, tmp_access->type);
		if(i%10000000 == 0) {
			printf("%ld,%ld,\t%ld,%ld,%ld\n", i /1000000, mpki, mpki_fetch, mpki_read, mpki_write);
			mpki = 0;
			mpki_fetch = 0;
			mpki_read = 0;
			mpki_write = 0;
		}

	}
	printf("Read count:\t%lu\n", read_count);
	printf("Write count:\t%lu\n", write_count);
	printf("Read miss count:\t%lu\n", read_miss_count);
	printf("Write miss count:\t%lu\n", write_miss_count);

	free(tmp_access);
}


























