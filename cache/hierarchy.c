#include <stdlib.h>

#include "hierarchy.h"
#include "bus.h"
#include "state.h"


struct CacheLevel* init_cache_level() {
    struct CacheLevel* new_level = malloc(sizeof(struct CacheLevel));
    new_level->size_caches_array = 8;
    new_level->amount_caches = 0;
    new_level->caches = malloc(8*sizeof(struct CacheState*));
    return new_level;
}

void free_cache_level(struct CacheLevel* level) {
    for(int i = 0; i < level->amount_caches; i++) {
        free_cachestate(level->caches[i]);
    }
    free_bus(level->bus);
    free(level);
}

struct CacheHierarchy* init_cache_hierarchy(int amount_cpus) {
    struct CacheHierarchy* hierarchy =  malloc(sizeof(struct CacheHierarchy));
    hierarchy->amount_cpus = amount_cpus;
    return hierarchy;
}

void free_cache_hierarchy(struct CacheHierarchy* hierarchy){
    for(int i = 0; i < hierarchy->amount_levels; i++) {
        free_cache_level(hierarchy->levels[i]);
    }
    free(hierarchy);
}

int add_cache_to_level(struct CacheLevel* level, struct CacheState* state) {

    if(level->amount_caches == level->size_caches_array) {
        //TODO implement
        printf("Not implemented!");
        return 1;
    }
    if(level->amount_caches == 1) {
        level->bus = init_bus();
        add_cache_to_bus(level->bus, level->caches[0]);
    }
    level->caches[level->amount_caches] = state;

    level->amount_caches++;
    if(level->amount_caches > 1 ) {
        add_cache_to_bus(level->bus, state);
    }
    return 0;
}


int add_level(struct CacheHierarchy* hierarchy, struct CacheLevel* level) {
    if(hierarchy->amount_levels == MAX_CACHE_LEVELS) {
        printf("Maximum amount of levels reached!\n");
        return 1;
    }
    if(hierarchy->amount_cpus < level->amount_caches) {
        printf("Amount of caches too large!\n");
        return 1;
    }
    if(hierarchy->amount_levels > 0) {
        struct CacheLevel* last_level = hierarchy->levels[hierarchy->amount_levels-1];
        if(level->amount_caches != 1 && level->amount_caches != last_level->amount_caches) {
            printf("Level has wrong amount of caches, previous level has: %d so should be that or 1\n", last_level->amount_caches);
            return 2;
        }

        for(int i = 0; i < last_level->amount_caches; i++) {
            // If new level only has a single cache all caches in the last_level have this cache as parent
            int parent_index = (level->amount_caches == 1 ? 0 : i);
            add_child(level->caches[parent_index], last_level->caches[i]);
        }
    }
    hierarchy->levels[hierarchy->amount_levels] = level;
    hierarchy->amount_levels++;
    return 0;
}

void access_cache_in_hierarchy(struct CacheHierarchy* hierarchy, uint64_t cpu, uint64_t address, uint64_t timestamp, bool write) {
    if(cpu >= hierarchy->amount_cpus) {
        printf("Cpu index higher than amount of cpus!\n");
        return;
    }
    if(hierarchy->levels[0]->amount_caches == 1) {
        access_cache(hierarchy->levels[0]->caches[0], address, timestamp, write);
    }else{
        access_cache(hierarchy->levels[0]->caches[cpu], address, timestamp, write);
    }
}
