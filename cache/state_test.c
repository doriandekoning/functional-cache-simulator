#ifdef TEST

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "state.h"
#include "lru.h"
#include "msi.h"
#include "test.h"
#include "memory/memory.h"

int tests_run = 0;

void test_cache_miss_func(bool write, uint64_t timestamp, uint64_t address) {
}

int test_setup_state_memory() {
    struct CacheState* created_state = setup_cachestate(NULL, false, 100, 10, 2, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    _assertEquals(created_state->size, 100);
    _assertEquals(created_state->line_size, 10);
    _assertEquals(created_state->higher_level_cache, NULL);
    _assertEquals(created_state->amount_lower_level_caches, 0);
    _assertEquals(created_state->associativity, 2);
    return 0;
}

int test_setup_state_higher_level_cache() {
    int higher_level_cache_size = 128;
    struct CacheState* higher_level_cache = setup_cachestate(NULL, false, higher_level_cache_size, 10, 1, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    struct CacheState* created_cache = setup_cachestate(higher_level_cache, false, higher_level_cache_size/8, 10, 1, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    _assertEquals(created_cache->higher_level_cache, higher_level_cache);
    _assertEquals(higher_level_cache->amount_lower_level_caches, 1);
    _assertEquals(higher_level_cache->lower_level_caches[0], created_cache);
    return 0;
}

int test_setup_state_higher_level_cache_size_not_correct() {
    int higher_level_cache_size = 128;
    struct CacheState* higher_level_cache = setup_cachestate(NULL, false, higher_level_cache_size, 10, 1, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    struct CacheState* created_cache = setup_cachestate(higher_level_cache, false, (higher_level_cache_size/8) + 1, 10, 1, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    _assertEquals(created_cache, NULL);
    _assertEquals(higher_level_cache->amount_lower_level_caches, 0);
    return 0;
}

int test_setup_state_invalid_associativity() {
    int higher_level_cache_size = 128;
    struct CacheState* higher_level_cache = setup_cachestate(NULL, false, higher_level_cache_size, 10, 1, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    // Associativity 0
    struct CacheState* created_cache = setup_cachestate(higher_level_cache, false, (higher_level_cache_size/8) + 1, 10, 0, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    _assertEquals(created_cache, NULL);
    // Associaitivty > size
    created_cache = setup_cachestate(higher_level_cache, false, (higher_level_cache_size/8) , 10, (higher_level_cache_size/8) + 10, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    _assertEquals(created_cache, NULL);

    return 0;
}

int test_setup_state_higher_level_cache_line_sizes_not_equal(){
    int higher_level_cache_size = 128;
    struct CacheState* higher_level_cache = setup_cachestate(NULL, false, higher_level_cache_size, 10, 1, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    struct CacheState* created_cache = setup_cachestate(higher_level_cache, false, (higher_level_cache_size/8) , 12, 1, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    _assertEquals(created_cache, NULL);
    _assertEquals(higher_level_cache->amount_lower_level_caches, 0);
    return 0;
}

int test_setup_state_higher_level_cache_has_64_lower_level_caches() {
    int higher_level_cache_size = 128;
    int amount_lower_level_caches = 7;
    struct CacheState* higher_level_cache = setup_cachestate(NULL, false, higher_level_cache_size, 10, 1, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    for(int i = 0; i < amount_lower_level_caches; i++) {
        struct CacheState* created_cache = setup_cachestate(higher_level_cache, false, (higher_level_cache_size/8) , 10, 1, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
        _assertEquals(created_cache->higher_level_cache, higher_level_cache);
    }
    _assertEquals(higher_level_cache->amount_lower_level_caches, amount_lower_level_caches);
    _assertEquals(higher_level_cache->cur_size_lower_level_caches_array, (amount_lower_level_caches + (8 -(amount_lower_level_caches%8))));
    return 0;
}

int test_setup_state_lines_initialized() {
    int size = 128;
    struct CacheState* cache = setup_cachestate(NULL, false, size, 10, 1, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    for(int i = 0; i < size; i++){
        _assertEquals(CACHELINE_STATE_INVALID, cache->lines[i].state);
    }
    free_cachestate(cache);
    return 0;
}


int test_free_cachestate() {
    int size = 128;
    struct CacheState* cache = setup_cachestate(NULL, false, size, 10, 1, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    free_cachestate(cache);
    return 0;
}

int test_calc_set_index_directly_mapped_cache() {
    int size = 64;
    int line_size = 8;
    struct CacheState* cache = setup_cachestate(NULL, false, size, line_size, 1, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    _assertEquals(0, CALCULATE_SET_INDEX(cache, 0*line_size));
    _assertEquals(1, CALCULATE_SET_INDEX(cache, 1*line_size));
    _assertEquals(0, CALCULATE_SET_INDEX(cache, 64*line_size));
    _assertEquals(3, CALCULATE_SET_INDEX(cache, 67*line_size));
    free_cachestate(cache);
}

int test_calc_set_index_fully_associative() {
    int size = 64;
    int line_size = 8;
    struct CacheState* cache = setup_cachestate(NULL, false, size, line_size, 64, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    _assertEquals(0, CALCULATE_SET_INDEX(cache, 0*line_size));
    _assertEquals(0, CALCULATE_SET_INDEX(cache, 1*line_size));
    _assertEquals(0, CALCULATE_SET_INDEX(cache, 49*line_size));
    _assertEquals(0, CALCULATE_SET_INDEX(cache, 64*line_size));
    _assertEquals(0, CALCULATE_SET_INDEX(cache, 67*line_size));
    _assertEquals(0, CALCULATE_SET_INDEX(cache, 270*line_size));
    free_cachestate(cache);
}


int test_calc_set_index_4way_associative() {
    int size = 64;
    int line_size = 8;
    struct CacheState* cache = setup_cachestate(NULL, false, size, line_size, 4, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    _assertEquals(0, CALCULATE_SET_INDEX(cache, 0*line_size));
    _assertEquals(4, CALCULATE_SET_INDEX(cache, 1*line_size));
    _assertEquals(16, CALCULATE_SET_INDEX(cache, 4*line_size));
    _assertEquals(0, CALCULATE_SET_INDEX(cache, 16*line_size));
    _assertEquals(4, CALCULATE_SET_INDEX(cache, 17*line_size));
    _assertEquals(0, CALCULATE_SET_INDEX(cache, 64*line_size));
    _assertEquals(3*4, CALCULATE_SET_INDEX(cache, 67*line_size));
    free_cachestate(cache);
}

int test_calc_tag_directly_mapped_cache() {
    int size = 64;
    int line_size = 8;
    struct CacheState* cache = setup_cachestate(NULL, false, size, line_size, 1, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    _assertEquals(0, CALCULATE_TAG(cache, 0*line_size));
    _assertEquals(0, CALCULATE_TAG(cache, 1*line_size));
    _assertEquals(1, CALCULATE_TAG(cache, 64*line_size));
    _assertEquals(1, CALCULATE_TAG(cache, 67*line_size));
    _assertEquals(203, CALCULATE_TAG(cache, ((203*64)+3)*line_size));
    free_cachestate(cache);
}

int test_calc_tag_fully_associative() {
    int size = 64;
    int line_size = 8;
    struct CacheState* cache = setup_cachestate(NULL, false, size, line_size, 64, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    _assertEquals(0, CALCULATE_TAG(cache, 0*line_size));
    _assertEquals(1, CALCULATE_TAG(cache, 1*line_size));
    _assertEquals(49, CALCULATE_TAG(cache, 49*line_size));
    _assertEquals(64, CALCULATE_TAG(cache, 64*line_size));
    _assertEquals(67, CALCULATE_TAG(cache, 67*line_size));
    _assertEquals(270, CALCULATE_TAG(cache, 270*line_size));
    free_cachestate(cache);
}


int test_calc_tag_4way_associative() {
    int size = 64;
    int line_size = 8;
    struct CacheState* cache = setup_cachestate(NULL, false, size, line_size, 4, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    _assertEquals(0, CALCULATE_TAG(cache, 0*line_size));
    _assertEquals(0, CALCULATE_TAG(cache, 1*line_size));
    _assertEquals(0, CALCULATE_TAG(cache, 4*line_size));
    _assertEquals(1, CALCULATE_TAG(cache, 16*line_size));
    _assertEquals(1, CALCULATE_TAG(cache, 17*line_size));
    _assertEquals(4, CALCULATE_TAG(cache, 64*line_size));
    _assertEquals(4, CALCULATE_TAG(cache, 67*line_size));
    free_cachestate(cache);
}


int test_perform_cache_access() {
    struct CacheState* cache = setup_cachestate(NULL, false, 64, 1, 4, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    access_cache(cache, 4, 10, true);
    int actual_line_loc = get_line_location_in_cache(cache, 4);
    _assert(actual_line_loc >= 16 && actual_line_loc < 20);
    return 0;
}


int test_perform_cache_access_not_found() {
    struct CacheState* cache = setup_cachestate(NULL, false, 64, 1, 4, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    _assert(get_line_location_in_cache(cache, 4) == -1);
    return 0;
}

int test_perform_cache_access_multiple() {
    struct CacheState* cache = setup_cachestate(NULL, false, 64, 1, 4, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    for(int i = 0; i < 4; i++) {
        access_cache(cache, i, 10+i, true);
    }
    for(int i = 0; i < 4; i++) {
        int actual_line_loc = get_line_location_in_cache(cache, i);
        _assert(actual_line_loc >= (i*4) && actual_line_loc < ((i+1)*4));
    }
    return 0;
}

int test_perform_cache_access_evict() {
    struct CacheState* cache = setup_cachestate(NULL, false, 64, 1, 4, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    for(int i = 0; i < 5; i++) {
        access_cache(cache, i*16, 10+i, true);
    }
    // The first should be evicted
    _assertEquals(-1, get_line_location_in_cache(cache, 0));

    for(int i = 1; i < 5; i++) {
        int actual_line_loc = get_line_location_in_cache(cache, i*16);
        _assert(actual_line_loc >= 0 && actual_line_loc < 4);
    }
    return 0;
}

int test_perform_cache_access_different_lines_no_evict() {
    struct CacheState* cache = setup_cachestate(NULL, false, 64, 1, 4, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    access_cache(cache, 0, 11, true);
    access_cache(cache, 16, 12, true);
    access_cache(cache, 64, 13, true);
    access_cache(cache, 1, 14, true);
    access_cache(cache, 2, 15, true);
    // The first should not be evicted
    _assert(get_line_location_in_cache(cache, 0) != -1);

    return 0;
}


int test_perform_cache_access_twice_same() {
    struct CacheState* cache = setup_cachestate(NULL, false, 64, 1, 4, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
     _assert(get_line_location_in_cache(cache, 0x11) == -1);
    access_cache(cache, 0x11, 0, true);
    _assert(get_line_location_in_cache(cache, 0x11 + 16) == -1);
    access_cache(cache, 0x11 + 16, 1, true);
    _assert(get_line_location_in_cache(cache, 0x11) != -1);
    _assert(get_line_location_in_cache(cache, 0x11 + 16) != -1);
    return 0;
}



int test_perform_cache_access_read_write() {
    struct CacheState* cache = setup_cachestate(NULL, false, 64, 1, 4, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    _assert(get_line_location_in_cache(cache, 0x11) == -1);
    access_cache(cache, 0x11, 0, false);
    _assertEquals(CACHELINE_STATE_SHARED, cache->lines[get_line_location_in_cache(cache, 0x11)].state);
    access_cache(cache, 0x11, 1, true);
    _assertEquals(CACHELINE_STATE_MODIFIED, cache->lines[get_line_location_in_cache(cache, 0x11)].state);
    return 0;
}


int test_perform_cache_access_write_read() {
    struct CacheState* cache = setup_cachestate(NULL, false, 64, 1, 4, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    _assertEquals(-1, get_line_location_in_cache(cache, 0x11));
    access_cache(cache, 0x11, 0, true);
    _assert(get_line_location_in_cache(cache, 0x11) != -1);
    _assertEquals(CACHELINE_STATE_MODIFIED, cache->lines[get_line_location_in_cache(cache, 0x11)].state);
    access_cache(cache, 0x11, 0, false);
    _assertEquals(CACHELINE_STATE_MODIFIED, cache->lines[get_line_location_in_cache(cache, 0x11)].state);
    return 0;
}



int test_perform_cache_access_evict_oldest() {
    struct CacheState* cache = setup_cachestate(NULL, false, 64, 1, 4, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    // Each cache set contains (CACHE_AMOUNT_LINES/AMOUNT_CACHE_SETS) lines, thus if we fill all those and add one additional one
    // the first should be evicted
    for(int i = 0; i < 5; i++) {
        access_cache(cache, 0x11 + (i*64), 0, true);
    }
    _assertEquals(-1, get_line_location_in_cache(cache, 0x11));
    //Start from 2 since the first and second are evicted by accesses above (the first in the loop and the one in the line directly above)
    for(int i = 2; i < 5; i++) {
        access_cache(cache, 0x11 + (i*64), 0, true);
        _assert(-1 != get_line_location_in_cache(cache, 0x11 + (i*64)));
    }
    return 0;
}


int test_perform_cache_access_evict_invalid() {
    struct CacheState* cache = setup_cachestate(NULL, false, 64, 1, 8, &find_line_to_evict_lru, &msi_coherency_protocol, &test_cache_miss_func);
    for(int i = 0; i < 6; i++) { // Fill cache set partially
        access_cache(cache, 0x11 + (i*64), i, false);
    }
    for(int i = 0; i < 6; i++) { // Fill cache set partially
        _assertEquals(CACHELINE_STATE_SHARED, cache->lines[get_line_location_in_cache(cache, 0x11 + (i*64))].state);
    }
    //Insert an additional line which should not evict anything
    access_cache(cache, 0x11 + (8*64), 10, false);

    for(int i = 0; i < 8; i++) { // Fill cache set partially
        if(i == 6)continue;
        if(i == 7)continue;
        access_cache(cache, 0x11 + (i*64), 100+i, false);
        _assertEquals(CACHELINE_STATE_SHARED, cache->lines[get_line_location_in_cache(cache, 0x11 + (i*64))].state);
    }
    return 0;
}




int test_state() {
    int failed_tests = 0;
    _test(test_setup_state_memory, "test_setup_state_memory");
    _test(test_setup_state_higher_level_cache, "test_setup_state_higher_level_cache");
    _test(test_setup_state_higher_level_cache_size_not_correct, "test_setup_state_higher_level_cache_size_not_correct");
    _test(test_setup_state_higher_level_cache_line_sizes_not_equal, "test_setup_state_higher_level_cache_line_sizes_not_equal");
    _test(test_setup_state_higher_level_cache_has_64_lower_level_caches, "test_setup_state_higher_level_cache_has_64_lower_level_caches");
    _test(test_setup_state_lines_initialized, "test_setup_state_lines_initialized");
    _test(test_free_cachestate, "test_free_cachestate");
    _test(test_setup_state_invalid_associativity, "test_setup_state_invalid_associativity");
    _test(test_calc_set_index_directly_mapped_cache, "test_calc_set_index_directly_mapped_cache");
    _test(test_calc_set_index_fully_associative, "test_calc_set_index_fully_associative");
    _test(test_calc_set_index_4way_associative, "test_calc_set_index_4way_associative");
    _test(test_calc_tag_directly_mapped_cache, "test_calc_set_index_directly_mapped_cache");
    _test(test_calc_tag_fully_associative, "test_calc_set_index_fully_associative");
    _test(test_calc_tag_4way_associative, "test_calc_set_index_4way_associative");

    _test(test_perform_cache_access, "test_perform_cache_access");
    _test(test_perform_cache_access_not_found, "test_perform_cache_access_not_found");
    _test(test_perform_cache_access_multiple, "test_perform_cache_access_multiple");
    _test(test_perform_cache_access_evict, "test_perform_cache_access_evict");

    _test(test_perform_cache_access_twice_same, "test_perform_cache_access_twice_same");
    _test(test_perform_cache_access_read_write, "test_perform_cache_access_read_write");
    _test(test_perform_cache_access_write_read, "test_perform_cache_access_write_read");
    _test(test_perform_cache_access_evict_oldest, "test_perform_cache_access_evict_oldest");
    _test(test_perform_cache_access_evict_invalid, "test_perform_cache_access_evict_invalid");
    // _test(test_write_on_other_cpu_invalidates, "test_write_on_other_cpu_invalidates");
    // _test(test_read_on_other_cpu_keeps_shared, "test_read_on_other_cpu_keeps_shared");
    // _test(test_setup_state_higher_level_cache_has_64_lower_level_caches, "test_setup_state_higher_level_cache_has_64_lower_level_caches");
    if(failed_tests == 0 ){
        printf("All tests passed in state_test!\n");
    } else {
        printf("One of the tests failed in state_test\n");
    }
    return 0;

}




#endif
