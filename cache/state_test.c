#include "state.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int tests_run = 0;

#define FAIL() printf("\n Failure in %s() line %d\n", __func__, __LINE__);
#define _assert(test) do {if (!(test)) { FAIL(); return 1; } } while(0)

#define _test(test, testname) do {int result = test(); failed_tests+= result; if(result) { printf("Failed %s!\n", testname);}else{printf("Passed %s\n", testname);}} while(0)
//;failed_tests+=result;if(result){printf("Test passed!\n"
//); } else {printf("Test %s failed!", "test");}} while(0)


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
    _test(test_perform_cache_access_twice_same, "test_perform_cache_access_twice_same");
    _test(test_perform_cache_access_twice_twice_same, "test_perform_cache_access_twice_twice_same");
    _test(test_perform_cache_access_evict_oldest, "test_perform_cache_access_evict_oldest");
    _test(test_perform_cache_access_read_write, "test_perform_cache_access_read_write");
    _test(test_perform_cache_access_write_read, "test_perform_cache_access_write_read");
    _test(test_perform_cache_access_evict_invalid, "test_perform_cache_access_evict_invalid");
    _test(test_write_on_other_cpu_invalidates, "test_write_on_other_cpu_invalidates");
    _test(test_read_on_other_cpu_keeps_shared, "test_read_on_other_cpu_keeps_shared");
    if(failed_tests == 0 ){
        printf("All tests passed!\n");
    } else {
        printf("One of the tests failed\n");
    }
    return 0;

}



