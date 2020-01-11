#ifndef __BUS_H
#define __BUS_H

struct Bus{
    struct CacheState** caches;
    int amount_caches;
    int cur_size_caches_array;
};

struct Bus* init_bus();
void free_bus(struct Bus* bus);
void add_cache(struct Bus* bus, struct CacheState* cache);
void handle_bus_event(struct Bus* bus, int event, uint64_t address);



#endif /* __BUS_H */
