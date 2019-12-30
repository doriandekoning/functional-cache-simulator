#include "multilevel.h"

//TODO maybe virt and physical address
bool cache_access(MultiLevelCacheState multilevel_cache_state, uint8_t cpu, uint64_t address, bool write) {

}


struct MultiLevelCacheState* init_cachestate(uint8_t amount_cpus, uint8_t amount_levels) {
    struct MultiLevelCacheState* state = malloc(sizeof(struct MultiLevelCacheState));
    state->amount_processors = amount_cpus;
    state->levels = amount_levels;
    state->states = malloc(sizeof(CacheState*) * amount_cpus * amount_levels);
}
