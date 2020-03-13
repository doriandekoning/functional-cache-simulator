#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "worker.h"
#include "cache/hierarchy.h"
#include "cache/lru.h"
#include "cache/msi.h"
#include "config.h"
#include "cache/state.h"
#include "simulator/distributed/output.h"
#include "simulator/distributed/mpi_datatype.h"

uint64_t total_miss = 0;


struct CacheMiss* output_buf;
int output_buf_idx;
int total_world_size;
MPI_Datatype mpi_output_datatype;

void cache_miss_func(bool write, uint64_t timestamp, uint64_t address) {
	// output_buf[output_buf_idx].addr = address;
	// output_buf[output_buf_idx].cpu = write; //TODO fix
	// output_buf[output_buf_idx].timestamp = timestamp;
	// output_buf_idx++;
	// if(output_buf_idx == 1024) {
	// 	debug_print("[WORKER]Sending output to output writer!\n");
	// 	if(MPI_Send(output_buf, 1024, mpi_output_datatype, total_world_size - 3, MPI_TAG_CACHE_MISS, MPI_COMM_WORLD) != MPI_SUCCESS ){
	// 		printf("Unable to send to output\n");
	// 	}
	// 	output_buf_idx = 0;
	// }
}

struct CacheHierarchy* setup_cache(int amount_cpus) {
	struct CacheHierarchy* hierarchy = init_cache_hierarchy(amount_cpus);
	struct CacheLevel* l1 = init_cache_level(amount_cpus, true);
	for(int i = 0; i < amount_cpus; i++) {
		struct CacheState* dcache = setup_cachestate(NULL, true, 8*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
		struct CacheState* icache = setup_cachestate(NULL, true, 8*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
		add_caches_to_level(l1, dcache, icache);
	}

	if(add_level(hierarchy, l1) != 0 ){
		exit(1);
	}

	struct CacheLevel* l2 = init_cache_level(amount_cpus, false);
	for(int i = 0; i < amount_cpus; i++) {
		struct CacheState* state = setup_cachestate(NULL, true, 256*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
		add_caches_to_level(l2, state, NULL);
	}
	if(add_level(hierarchy, l2) != 0 ){
		exit(1);
	}
	struct CacheLevel* l3 = init_cache_level(amount_cpus, false);
	struct CacheState* state = setup_cachestate(NULL, true, 8*1024*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
	add_caches_to_level(l3, state, NULL);
	if(add_level(hierarchy, l3) != 0 ){
		exit(1);
	}
	return hierarchy;
}


int run_worker(int world_size, int world_rank, int amount_cpus) {
	debug_print("[WORKER]Worker started\n");
    cache_access* buffer;
    int amount_sim_caches = 16;
    #ifdef SIMULATE_CACHE
    //struct CacheHierarchy* cache = setup_cache(amount_cpus);
    struct CacheHierarchy** cache = malloc(sizeof(struct CacheHierarchy*) * amount_sim_caches); 
    for(int i = 0; i < amount_sim_caches; i++ ){
	cache[i] = setup_cache(amount_cpus);
    }
    #endif
	debug_print("Worker has setup cache\n");

    buffer = malloc(sizeof(cache_access)*1024);
	debug_print("Running worker\n");

	output_buf = malloc(1024*sizeof(struct CacheMiss));
	total_world_size = world_size;
	output_buf_idx = 0;
	if(get_mpi_cache_miss_datatype(&mpi_output_datatype)) {
		printf("Unable to setup datatype!\n");
		return 1;
	}
    while(1) {
		debug_print("[WORKER]Worker waiting to receive buffer!\n");
        if(MPI_Recv(buffer, sizeof(cache_access)*1024, MPI_CHAR, world_size - 2, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE)) {
            printf("unable to receive!\n");
            return 1;
        }
		debug_print("[WORKER]Worker received buffer\n");
        #ifdef SIMULATE_CACHE
        for(int i = 0; i < 1024;i++) {
			// debug_printf("[WORKER]doing cache simulation, cpu:%u, add: %lx, tick: %lx, type:%d\n", buffer[i].cpu, buffer[i].address, buffer[i].tick, buffer[i].type);
	for(int j = 0; j < amount_sim_caches; j++){
            if(access_cache_in_hierarchy(cache[j], buffer[i].cpu, buffer[i].address, buffer[i].tick, buffer[i].type)) {
                printf("Unable to access cache!\n");
                return 1;
            }
	}
			// debug_print("[WORKER]finished cache simulation\n");
        }
        #endif

    }
}
