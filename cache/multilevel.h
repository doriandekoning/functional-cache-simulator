#ifndef MULTILEVEL_H
#define MULTILEVEL


struct MultiLevelCacheState {
    uint8_t amount_processors;
    uint8_t levels;
    CacheState* [][] states; // First dimension specifies the level second the cpu in the level, multiple entries in this array can point to the same CacheState
};

// init_cachestate initalizes a MultiLevelCacheState struct but does not initialize the CacheStates for each level
// these should be initalized by the caller with with the correct size and mapping before it is used.
struct MultiLevelCacheState* init_cachestate(uint8_t amount_proccessors, uint8_t amount_levels);


bool cache_access(MultiLevelCacheState multilevel_cache_state, uint8_t cpu, uint64_t address, bool write);


#endif /* MULTILEVEL */
