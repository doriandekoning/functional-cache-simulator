#ifndef _HIERARCHY_H
#define _HIERARCHY_H

#include "state.h"
#include "bus.h"

#define MAX_CACHE_LEVELS 8

struct CacheLevel {
    int amount_caches;
    int size_caches_array;
    struct CacheState** caches;
    struct Bus* bus;
};

struct CacheHierarchy {
    int amount_cpus;
    int amount_levels;
    struct CacheLevel* levels[MAX_CACHE_LEVELS];
};

struct CacheLevel* init_cache_level();
void free_cache_level(struct CacheLevel* level);
struct CacheHierarchy* init_cache_hierarchy(int amount_cpus);
void free_cache_hierachy(struct CacheHierarchy* hierarchy);

int add_level(struct CacheHierarchy* hierarchy, struct CacheLevel* level);
int add_cache_to_level(struct CacheLevel* level, struct CacheState* state);
void access_cache_in_hierarchy(struct CacheHierarchy* hierarchy, uint64_t cpu, uint64_t address, uint64_t timestamp, bool write);

#endif /* _HIERARCHY_H */

