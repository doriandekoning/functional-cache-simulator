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

uint64_t total_miss = 0;

void cache_miss_func(bool write, uint64_t timestamp, uint64_t address) {
    total_miss++;
    if(total_miss %1000000  == 0) {
        printf("%luM misses\n",  total_miss/1000000);
    }
}

struct CacheHierarchy* setup_cache() {
	struct CacheHierarchy* hierarchy = init_cache_hierarchy(AMOUNT_SIMULATED_PROCESSORS);
	struct CacheLevel* l1 = init_cache_level(AMOUNT_SIMULATED_PROCESSORS, true);
	for(int i = 0; i < AMOUNT_SIMULATED_PROCESSORS; i++) {
		struct CacheState* dcache = setup_cachestate(NULL, true, 8*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
		struct CacheState* icache = setup_cachestate(NULL, true, 8*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
		add_caches_to_level(l1, dcache, icache);
	}

	if(add_level(hierarchy, l1) != 0 ){
		exit(1);
	}

	struct CacheLevel* l2 = init_cache_level(AMOUNT_SIMULATED_PROCESSORS, false);
	for(int i = 0; i < AMOUNT_SIMULATED_PROCESSORS; i++) {
		struct CacheState* state = setup_cachestate(NULL, true, 256*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
		add_caches_to_level(l2, state, NULL);
	}
	if(add_level(hierarchy, l2) != 0 ){
		exit(1);
	}
	struct CacheLevel* l3 = init_cache_level(AMOUNT_SIMULATED_PROCESSORS, false);
	struct CacheState* state = setup_cachestate(NULL, true, 8*1024*1024, CACHE_LINE_SIZE, ASSOCIATIVITY, &find_line_to_evict_lru, &msi_coherency_protocol, &cache_miss_func);
	add_caches_to_level(l3, state, NULL);
	if(add_level(hierarchy, l3) != 0 ){
		exit(1);
	}
	return hierarchy;
}


int run_worker(int world_size, int world_rank) {
    cache_access* buffer;
    #ifdef SIMULATE_CACHE
    struct CacheHierarchy* cache = setup_cache();
    #endif

    buffer = malloc(sizeof(cache_access)*1024);
    while(1) {
        if(MPI_Recv(buffer, sizeof(cache_access)*1024, MPI_CHAR, world_size - 2, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE)) {
            printf("unable to receive!\n");
            return 1;
        }
        #ifdef SIMULATE_CACHE
        for(int i = 0; i < 1024;i++) {
            if(access_cache_in_hierarchy(cache, buffer[i].cpu, buffer[i].address, buffer[i].tick, buffer[i].type)) {
                printf("Unable to access cache!\n");
                return 1;
            }
        }
        #endif

    }
}
