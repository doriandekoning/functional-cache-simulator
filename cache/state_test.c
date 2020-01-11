#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "state.h"
#include "test.h"
#include "memory/memory.h"

int tests_run = 0;


int test_setup_state_memory() {
    struct Memory* mem = init_memory();
    struct CacheState* created_state = setup_cache(NULL, mem, false, 100, 10);
    _assertEquals(created_state->size, 100);
    _assertEquals(created_state->line_size, 10);
    _assertEquals(created_state->memory, mem);
    _assertEquals(created_state->parent_cache, NULL);
    _assertEquals(created_state->amount_children, 0);
    return 0;
}

int test_setup_state_parent_cache() {
    int parent_size = 128;
    struct CacheState* parent = setup_cache(NULL, init_memory(), false, parent_size, 10);
    struct CacheState* created_cache = setup_cache(parent, NULL, false, parent_size/8, 10);
    _assertEquals(created_cache->parent_cache, parent);
    _assertEquals(parent->amount_children, 1);
    _assertEquals(parent->children[0], created_cache);
    return 0;
}

int test_setup_state_parent_size_not_correct() {
    int parent_size = 128;
    struct CacheState* parent = setup_cache(NULL, init_memory(), false, parent_size, 10);
    struct CacheState* created_cache = setup_cache(parent, NULL, false, (parent_size/8) + 1, 10);
    _assertEquals(created_cache, NULL);
    _assertEquals(parent->amount_children, 0);
    return 0;
}

int test_setup_state_parent_cache_line_sizes_not_equal(){
    int parent_size = 128;
    struct CacheState* parent = setup_cache(NULL, init_memory(), false, parent_size, 10);
    struct CacheState* created_cache = setup_cache(parent, NULL, false, (parent_size/8) , 12);
    _assertEquals(created_cache, NULL);
    _assertEquals(parent->amount_children, 0);
    return 0;
}

int test_setup_state_parent_has_64_children() {
    int parent_size = 128;
    int amount_children = 7;
    struct CacheState* parent = setup_cache(NULL, init_memory(), false, parent_size, 10);
    for(int i = 0; i < amount_children; i++) {
        struct CacheState* created_cache = setup_cache(parent, NULL, false, (parent_size/8) , 10);
        _assertEquals(created_cache->parent_cache, parent);
    }
    _assertEquals(parent->amount_children, amount_children);
    _assertEquals(parent->cur_size_children_array, (amount_children + (8 -(amount_children%8))));
    return 0;
}




int test_perform_cache_access_twice_same() {
    CacheState state = calloc(AMOUNT_SIMULATED_PROCESSORS * CACHE_AMOUNT_LINES * ASSOCIATIVITY,  sizeof(struct CacheLine));
    _assert(!perform_cache_access(state, 0, 0x11, true));
    _assert(perform_cache_access(state, 0, 0x11, true));
    return 0;
}

int test_perform_cache_access_twice_twice_same() {
    CacheState state = calloc(AMOUNT_SIMULATED_PROCESSORS * CACHE_AMOUNT_LINES * ASSOCIATIVITY,  sizeof(struct CacheLine));
    _assert(!perform_cache_access(state, 0, 0x11, true));
    _assert(!perform_cache_access(state, 0, 0x11 + (CACHE_AMOUNT_LINES/AMOUNT_CACHE_SETS), true));
    _assert(perform_cache_access(state, 0, 0x11, true));
    _assert(perform_cache_access(state, 0, 0x11 + (CACHE_AMOUNT_LINES/AMOUNT_CACHE_SETS), true));
    return 0;
}

int test_perform_cache_access_read_write() {
    CacheState state = calloc(AMOUNT_SIMULATED_PROCESSORS * CACHE_AMOUNT_LINES * ASSOCIATIVITY,  sizeof(struct CacheLine));
    _assert(STATE_INVALID == get_line_state(state, 0, 0x11, NULL));
    _assert(!perform_cache_access(state, 0, 0x11, false));
    _assert(STATE_SHARED == get_line_state(state, 0, 0x11, NULL));
    _assert(perform_cache_access(state, 0, 0x11, true));
    _assert(STATE_MODIFIED == get_line_state(state, 0, 0x11, NULL));
    return 0;
}

int test_perform_cache_access_write_read() {
    CacheState state = calloc(AMOUNT_SIMULATED_PROCESSORS * CACHE_AMOUNT_LINES * ASSOCIATIVITY, sizeof(struct CacheLine));
    _assert(STATE_INVALID == get_line_state(state, 0, 0x11, NULL));
    _assert(!perform_cache_access(state, 0, 0x11, true));
    _assert(STATE_MODIFIED == get_line_state(state, 0, 0x11, NULL));
    _assert(perform_cache_access(state, 0, 0x11, false));
    _assert(STATE_MODIFIED == get_line_state(state, 0, 0x11, NULL));
    return 0;
}

int test_perform_cache_access_evict_oldest() {
    CacheState state = calloc(AMOUNT_SIMULATED_PROCESSORS * CACHE_AMOUNT_LINES * ASSOCIATIVITY, sizeof(struct CacheLine));
    // Each cache set contains (CACHE_AMOUNT_LINES/AMOUNT_CACHE_SETS) lines, thus if we fill all those and add one additional one
    // the first should be evicted
    for(int i = 0; i < (CACHE_AMOUNT_LINES/AMOUNT_CACHE_SETS) +1; i++) {
        _assert(!perform_cache_access(state, 0, 0x11 + (i*CACHE_AMOUNT_LINES), true));
    }
    _assert(!perform_cache_access(state, 0, 0x11, true));
    //Start from 2 since the first and second are evicted by accesses above (the first in the loop and the one in the line directly above)
    for(int i = 2; i < (CACHE_AMOUNT_LINES/AMOUNT_CACHE_SETS); i++) {
        _assert(perform_cache_access(state, 0, 0x11 + (i*CACHE_AMOUNT_LINES), true));
    }
    return 0;
}

int test_perform_cache_access_evict_invalid() {
    CacheState state = calloc(AMOUNT_SIMULATED_PROCESSORS * CACHE_AMOUNT_LINES * ASSOCIATIVITY, sizeof(struct CacheLine));
    for(int i = 0; i < (CACHE_AMOUNT_LINES/AMOUNT_CACHE_SETS) - 2; i++) { // Fill cache set partially
        _assert(!perform_cache_access(state, 0, 0x11 + (i*CACHE_AMOUNT_LINES), false));
    }
    for(int i = 0; i < (CACHE_AMOUNT_LINES/AMOUNT_CACHE_SETS) - 2; i++) { // Fill cache set partially
        _assert(STATE_SHARED == get_line_state(state, 0, 0x11 + (i*CACHE_AMOUNT_LINES), NULL));
    }
    //Insert an additional line which should not evict anything
    perform_cache_access(state, 0, 0x11 + ((CACHE_AMOUNT_LINES/AMOUNT_CACHE_SETS)*CACHE_AMOUNT_LINES), false);

    for(int i = 0; i < (CACHE_AMOUNT_LINES/AMOUNT_CACHE_SETS); i++) { // Fill cache set partially
        if(i == (CACHE_AMOUNT_LINES/AMOUNT_CACHE_SETS)-2)continue;
        if(i == (CACHE_AMOUNT_LINES/AMOUNT_CACHE_SETS)-1)continue;
        _assert(perform_cache_access(state, 0, 0x11 + (i*CACHE_AMOUNT_LINES), false));
        _assert(STATE_SHARED == get_line_state(state, 0, 0x11 + (i*CACHE_AMOUNT_LINES), NULL));
    }
    return 0;
}

int test_write_on_other_cpu_invalidates() {
    CacheState state = calloc(AMOUNT_SIMULATED_PROCESSORS * CACHE_AMOUNT_LINES * ASSOCIATIVITY, sizeof(struct CacheLine));
    //Insert entry into cache
    uint64_t address = 0x1234abc;
    perform_cache_access(state, 0, address, false);
    _assert(STATE_SHARED == get_line_state(state, 0, address, NULL));

    // Write entry on other cpu
    perform_cache_access(state, 1, address, true);
    _assert(STATE_INVALID == get_line_state(state, 0, address, NULL));
    _assert(STATE_MODIFIED == get_line_state(state, 1, address, NULL));
    return 0;
}



int test_read_on_other_cpu_keeps_shared() {
    CacheState state = calloc(AMOUNT_SIMULATED_PROCESSORS * CACHE_AMOUNT_LINES * ASSOCIATIVITY, sizeof(struct CacheLine));
    //Insert entry into cache
    uint64_t address = 0x1234abc;
    perform_cache_access(state, 0, address, false);
    _assert(STATE_SHARED == get_line_state(state, 0, address, NULL));

    // Write entry on other cpu
    perform_cache_access(state, 1, address, false);
    _assert(STATE_SHARED == get_line_state(state, 0, address, NULL));
    _assert(STATE_SHARED == get_line_state(state, 1, address, NULL));
    return 0;
}


int main(int argc, char **argv) {
    init_cachestate_masks(12, OFFSET_BITS);
    int failed_tests = 0;
    _test(test_setup_state_memory, "test_setup_state_memory");
    _test(test_setup_state_parent_cache, "test_setup_state_parent_cache");
    _test(test_setup_state_parent_size_not_correct, "test_setup_state_parent_size_not_correct");
    _test(test_setup_state_parent_cache_line_sizes_not_equal, "test_setup_state_parent_cache_line_sizes_not_equal");
    _test(test_perform_cache_access_twice_same, "test_perform_cache_access_twice_same");
    _test(test_perform_cache_access_twice_twice_same, "test_perform_cache_access_twice_twice_same");
    _test(test_perform_cache_access_evict_oldest, "test_perform_cache_access_evict_oldest");
    _test(test_perform_cache_access_read_write, "test_perform_cache_access_read_write");
    _test(test_perform_cache_access_write_read, "test_perform_cache_access_write_read");
    _test(test_perform_cache_access_evict_invalid, "test_perform_cache_access_evict_invalid");
    _test(test_write_on_other_cpu_invalidates, "test_write_on_other_cpu_invalidates");
    _test(test_read_on_other_cpu_keeps_shared, "test_read_on_other_cpu_keeps_shared");
    _test(test_setup_state_parent_has_64_children, "test_setup_state_parent_has_64_children");
    if(failed_tests == 0 ){
        printf("All tests passed!\n");
    } else {
        printf("One of the tests failed\n");
    }
    return 0;

}



