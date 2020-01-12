#ifndef __BUS_H
#define __BUS_H

#include "state.h"

struct Bus{
    struct CacheState** caches;
    int amount_caches;
    int cur_size_caches_array;
};

struct Bus* init_bus();
void free_bus(struct Bus* bus);
void add_cache_to_bus(struct Bus* bus, struct CacheState* cache);
void handle_bus_event(struct Bus* bus, int event, uint64_t address, struct CacheState* origin_cache);



#endif /* __BUS_H */
