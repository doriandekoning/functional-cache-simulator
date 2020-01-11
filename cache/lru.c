#include <cache/state.h>

int find_line_to_evict_lru(struct CacheState* cache, uint64_t address) {
    int set_idx = CALCULATE_SET_INDEX(cache, address);
    uint64_t smallest_timestamp = cache->lines[set_idx].last_used;
    int evict_idx = set_idx;
    for(int i = set_idx; i < (set_idx + cache->associativity);i++) {
        if(cache->lines[i].state == CACHELINE_STATE_INVALID) {
            return i;
        }
        if(cache->lines[set_idx].last_used < smallest_timestamp){
            smallest_timestamp = cache->lines[i].last_used;
            evict_idx = i;
        }
    }
    return evict_idx;
}
