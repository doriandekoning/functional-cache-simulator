#ifndef OUPUT_H
#define OUTPUT_H

struct CacheMiss {
    uint64_t addr;
    uint64_t timestamp;
    uint8_t cpu;
};

int run_output(int world_size, int world_rank, int amount_workers, char* output);

#endif /* OUTPUT_H */
