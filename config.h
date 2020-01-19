
#ifndef CONFIG_H
#define CONFIG_H



#define CACHE_LINE_SIZE 8
#define ASSOCIATIVITY 8


#define AMOUNT_SIMULATED_PROCESSORS 4
#define SIMULATION_LIMIT    5000000000
#define MESSAGE_BATCH_SIZE  1000
#define INPUT_BATCH_SIZE 1024

#define INPUT_READER_BATCH_SIZE 1024


#ifdef DEBUG
  #define debug_printf(fmt, args...) printf("%s:%s:%d:\t "fmt, __FILE__, __FUNCTION__, __LINE__, args)
  #define debug_print(str) printf("%s:%s:%d: "str, __FILE__, __FUNCTION__, __LINE__)
#else
  #define debug_printf(fmt, args...) {}
  #define debug_print(str) {}
#endif

#endif /* CONFIG_H */
