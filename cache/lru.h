#ifndef __LRU_H
#define __LRU_H


#include <cache/state.h>

int find_line_to_evict_lru(struct CacheState* cache, uint64_t address);


#endif
