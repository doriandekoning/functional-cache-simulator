
#ifndef CONFIG_H
#define CONFIG_H

#define CACHE_LINE_SIZE 64
#define CACHE_SIZE 8388608 // The size of the simulated cache (in 64byte lines)
#define ASSOCIATIVITY 8
#define AMOUNT_CACHE_SETS (CACHE_SIZE / ASSOCIATIVITY)
#define AMOUNT_SIMULATED_PROCESSORS 8
#define SIMULATION_LIMIT    50000000
#define MESSAGE_BATCH_SIZE  1000


#endif /* CONFIG_H */
