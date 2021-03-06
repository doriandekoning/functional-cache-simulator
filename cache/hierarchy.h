#ifndef _HIERARCHY_H
#define _HIERARCHY_H

#include <stdbool.h>

#include "state.h"
#include "bus.h"

#define MAX_CACHE_LEVELS 8

struct CacheLevel {
    size_t amount_caches;
    size_t size_caches_array;
    size_t size_instruction_caches_array;
    struct CacheState** caches;
    struct CacheState** instruction_caches;
    struct Bus* bus;
    bool has_instruction_caches;
};

struct CacheHierarchy {
    size_t amount_cpus;
    size_t amount_levels;
    struct CacheLevel* levels[MAX_CACHE_LEVELS];
};

struct CacheLevel* init_cache_level(size_t max_caches_amount, bool has_instruction_caches);
void free_cache_level(struct CacheLevel* level);
struct CacheHierarchy* init_cache_hierarchy(size_t amount_cpus);
void free_cache_hierachy(struct CacheHierarchy* hierarchy);

int add_level(struct CacheHierarchy* hierarchy, struct CacheLevel* level);
int add_caches_to_level(struct CacheLevel* level, struct CacheState* data_cache, struct CacheState* instruction_cache);
int access_cache_in_hierarchy(struct CacheHierarchy* hierarchy, uint64_t cpu, uint64_t address, uint64_t timestamp, int type);

#endif /* _HIERARCHY_H */

