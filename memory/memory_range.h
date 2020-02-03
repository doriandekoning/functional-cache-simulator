#ifndef _MEMORY_RANGE
#define _MEMORY_RANGE


struct MemoryRange {
    uint64_t start_addr;
    uint64_t end_addr;
    struct MemoryRange* next;
};

struct MemoryRange* read_memory_ranges(FILE* memory_ranges_file);
struct Memory* get_memory_for_address(int amount_memories, struct Memory* memories, uint64_t address);

#endif
