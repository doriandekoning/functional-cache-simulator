#include <stdlib.h>
#include <stdio.h>


#include "hierarchy.h"
#include "bus.h"
#include "state.h"


struct CacheLevel* init_cache_level(size_t max_caches_amount, bool has_instruction_caches) {
    struct CacheLevel* new_level = malloc(sizeof(struct CacheLevel));
    size_t icaches_amount = has_instruction_caches ? max_caches_amount : 0;
    new_level->size_caches_array = max_caches_amount;
    new_level->size_instruction_caches_array = icaches_amount;
    new_level->amount_caches = 0;
    new_level->caches = malloc(max_caches_amount*sizeof(struct CacheState*));
    new_level->instruction_caches = malloc(icaches_amount*sizeof(struct CacheState*));
    new_level->has_instruction_caches = has_instruction_caches;
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
    hierarchy->amount_levels = 0;
    return hierarchy;
}

void free_cache_hierarchy(struct CacheHierarchy* hierarchy){
    for(int i = 0; i < hierarchy->amount_levels; i++) {
        free_cache_level(hierarchy->levels[i]);
    }
    free(hierarchy);
}

int add_caches_to_level(struct CacheLevel* level, struct CacheState* data_cache, struct CacheState* instruction_cache) {

    if(level->amount_caches == level->size_caches_array) {
        //TODO implement
        printf("Not implemented!");
        return 1;
    }
    if(instruction_cache == NULL && level->has_instruction_caches) {
        printf("Instruction cache missing!\n");
        return 2;
    }else if(instruction_cache != NULL && !level->has_instruction_caches) {
        printf("Instruction cache supplied but none required!\n");
        return 3;
    }
    if(level->amount_caches == 1) {
        level->bus = init_bus();
        add_cache_to_bus(level->bus, level->caches[0]);
        if(level->has_instruction_caches) {
            add_cache_to_bus(level->bus, level->instruction_caches[0]);
        }
    }
    level->caches[level->amount_caches] = data_cache;
    if(level->has_instruction_caches) {
        level->instruction_caches[level->amount_caches] = instruction_cache;
    }
    level->amount_caches++;
    if(level->amount_caches > 1) {
        add_cache_to_bus(level->bus, data_cache);
        if(level->has_instruction_caches) {
            add_cache_to_bus(level->bus, instruction_cache);
        }
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
            printf("Level has wrong amount of caches, previous level has: %d so should be that or 1 but is: %d\n", last_level->amount_caches,  level->amount_caches);
            return 2;
        }
        for(int i = 0; i < last_level->amount_caches; i++) {
            // If new level only has a single cache all caches in the last_level have this cache as higher level cache
            add_lower_level_cache(level->caches[level->amount_caches == 1 ? 0 : i], last_level->caches[i]);
            if(level->has_instruction_caches) {
                add_lower_level_cache(level->instruction_caches[level->amount_caches == 1 ? 0 : i], last_level->instruction_caches[i]);
            }
        }
    }
    hierarchy->levels[hierarchy->amount_levels] = level;
    hierarchy->amount_levels++;
    return 0;
}

int access_cache_in_hierarchy(struct CacheHierarchy* hierarchy, uint64_t cpu, uint64_t address, uint64_t timestamp, int type) {
    if(cpu >= hierarchy->amount_cpus) {
        printf("Cpu index higher than amount of cpus!\n");
        return 1;
    }
    if(type < CACHE_EVENT_NONE || type > CACHE_EVENT_MAX) {
        printf("Unknown cache event type: %d\n", type);
        return 1;
    }
    int cpu_idx = (hierarchy->levels[0]->amount_caches == 1) ? 0 : cpu;
    if(type == CACHE_EVENT_INSTRUCTION_FETCH) {
        access_cache(hierarchy->levels[0]->instruction_caches[cpu_idx], address, timestamp, false);
    }else{
        access_cache(hierarchy->levels[0]->caches[cpu_idx], address, timestamp, type == CACHE_EVENT_WRITE);
    }
    return 0;
}
