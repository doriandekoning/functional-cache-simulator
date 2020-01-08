
#ifndef CONFIG_H
#define CONFIG_H



#define CACHE_LINE_SIZE *((void*)0) // deprecated use CACHE_LINE_SIZE_BITS
#define CACHE_LINE_SIZE_BITS 6
#define CACHE_AMOUNT_LINES 8388608 // The size of the simulated cache (in 64byte lines) #PREVIOUSLY CACHE_SIZE
#define ASSOCIATIVITY 8
#define AMOUNT_CACHE_SETS (CACHE_AMOUNT_LINES / ASSOCIATIVITY)


#define AMOUNT_SIMULATED_PROCESSORS 4
#define SIMULATION_LIMIT    100000000
#define MESSAGE_BATCH_SIZE  1000
#define INPUT_BATCH_SIZE 1024

#define INPUT_READER_BATCH_SIZE 1024

#define VIRT_TO_PHYS_TRANSLATION 1

#ifdef DEBUG
  #define debug_printf(fmt, args...) printf("%s:%s:%d:\t "fmt, __FILE__, __FUNCTION__, __LINE__, args)
  #define debug_print(str) printf("%s:%s:%d: "str, __FILE__, __FUNCTION__, __LINE__)
#else
  #define debug_printf(fmt, args...) {}
  #define debug_print(str) {}
#endif

#endif /* CONFIG_H */
