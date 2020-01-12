#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "state.h"
#include "hierarchy.h"
#include "lru.h"
#include "msi.h"
#include "test.h"
#include "memory/memory.h"

struct CoherencyProtocol msi_coherency_protocol_hierarchy_test = {
    .new_state_func = &new_state_msi,
    .flush_needed_on_evict = &flush_needed_on_eviction_msi,
};

int get_state_of_line(struct CacheHierarchy* hierarchy, int level, int cpu, uint64_t addr) {
    int line_idx = get_line_location_in_cache(hierarchy->levels[level]->caches[cpu], addr);
    if(line_idx < 0) {
        return -1;
    }
    return hierarchy->levels[level]->caches[cpu]->lines[line_idx].state;
}

struct CacheHierarchy* setup_single_level_multi_cache_hierarchy() {
    struct CacheHierarchy* hierarchy = init_cache_hierarchy(8);
    struct CacheLevel* new_level = init_cache_level(8);
    for(int i = 0; i < 8; i++) {
        struct CacheState* cache = setup_cachestate(NULL, false, 64, 1, 8, &find_line_to_evict_lru, &msi_coherency_protocol_hierarchy_test);
        add_cache_to_level(new_level, cache);
    }
    add_level(hierarchy, new_level);
    return hierarchy;
}

struct CacheHierarchy* setup_two_level_multi_cache_hierarchy() {
    struct CacheHierarchy* hierarchy = init_cache_hierarchy(8);
    struct CacheLevel* l1 = init_cache_level(8);
    for(int i = 0; i < 8; i++) {
        struct CacheState* cache = setup_cachestate(NULL, false, 64, 1, 2, &find_line_to_evict_lru, &msi_coherency_protocol_hierarchy_test);
        add_cache_to_level(l1, cache);
    }
    add_level(hierarchy, l1);
    struct CacheLevel* l2 = init_cache_level(8);
    for(int i = 0; i < 8; i++) {
        struct CacheState* cache = setup_cachestate(NULL, false, 64, 1, 2, &find_line_to_evict_lru, &msi_coherency_protocol_hierarchy_test);
        add_cache_to_level(l2, cache);
    }

    add_level(hierarchy, l2);
    return hierarchy;
}

int test_init_cache_level() {
    struct CacheLevel* new_level = init_cache_level(8);
    _assertEquals(0, new_level->amount_caches);
    return 0;
}


int test_write_on_other_cpu_invalidates() {
    struct CacheHierarchy* hierarchy = setup_single_level_multi_cache_hierarchy();

    //Insert entry into cache
    uint64_t address = 0x1234abc;
    access_cache_in_hierarchy(hierarchy, 1, address, 10, false);
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 0, 1, address));

    // Write entry on other cpu
    access_cache_in_hierarchy(hierarchy, 2, address, 11, true);
    _assertEquals(CACHELINE_STATE_INVALID, get_state_of_line(hierarchy, 0, 1, address));
    _assertEquals(CACHELINE_STATE_MODIFIED, get_state_of_line(hierarchy, 0, 2, address));
    return 0;
}


int test_read_on_other_cpu_stays_shared() {
    struct CacheHierarchy* hierarchy = setup_single_level_multi_cache_hierarchy();

    //Insert entry into cache
    uint64_t address = 0x1234abc;
    access_cache_in_hierarchy(hierarchy, 0, address, 10, false);
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 0, 0, address));

    // Write entry on other cpu
    access_cache_in_hierarchy(hierarchy, 1, address, 11, false);
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 0, 0, address));
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 0, 1, address));
    return 0;
}

int test_higher_level_should_invalidate_lower_level_cache_on_evict() {
    struct CacheHierarchy* hierarchy = setup_two_level_multi_cache_hierarchy();

    //Insert entry into cache
    uint64_t address = 0x1234abc;
    access_cache_in_hierarchy(hierarchy, 0, address, 10, false);
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 0, 0, address));
    access_cache_in_hierarchy(hierarchy, 0, address + 32, 11, false);
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 0, 0, address + 32));

    // Place address in front of LRU in L1 (it is still in back at L2 since this access hit L1)
    access_cache_in_hierarchy(hierarchy, 0, address, 12, false);

    // This should evict address from L2
    access_cache(hierarchy->levels[1]->caches[0], address + 64,  13, true);

    _assertEquals(-1, get_state_of_line(hierarchy, 0, 0, address + 64));
    _assertEquals(-1, get_state_of_line(hierarchy, 0, 0, address + 64));

    return 0;
}

int test_multilevel() {
    struct CacheHierarchy* hierarchy = setup_two_level_multi_cache_hierarchy();

    //Insert entry into cache
    uint64_t address = 0x1234abc;
    access_cache_in_hierarchy(hierarchy, 0, address, 10, false);
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 0, 0, address));
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 1, 0, address));
    access_cache_in_hierarchy(hierarchy, 0, address + 32, 11, false);
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 0, 0, address + 32));
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 1, 0, address + 32));
    access_cache_in_hierarchy(hierarchy, 0, address + 64, 12, false);

    _assertEquals(-1, get_state_of_line(hierarchy, 0, 0, address));
    _assertEquals(-1, get_state_of_line(hierarchy, 1, 0, address));
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 0, 0, address + 32));
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 1, 0, address + 32));
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 0, 0, address + 64));
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 1, 0, address + 64));

    return 0;
}

int test_multilevel_with_llc() {
    struct CacheHierarchy* hierarchy = setup_two_level_multi_cache_hierarchy();
    struct CacheState* cache = setup_cachestate(NULL, false, 512, 1, 8, &find_line_to_evict_lru, &msi_coherency_protocol_hierarchy_test);
    struct CacheLevel* llc = init_cache_level(1);
    add_cache_to_level(llc, cache);
    add_level(hierarchy, llc);

    //Insert entry into cache
    uint64_t address = 0x1234abc;
    access_cache_in_hierarchy(hierarchy, 0, address, 10, false);
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 0, 0, address));
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 1, 0, address));
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 2, 0, address));
    access_cache_in_hierarchy(hierarchy, 0, address + 32, 11, false);
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 0, 0, address + 32));
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 1, 0, address + 32));
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 2, 0, address + 32));
    access_cache_in_hierarchy(hierarchy, 0, address + 64, 12, false);

    _assertEquals(-1, get_state_of_line(hierarchy, 0, 0, address));
    _assertEquals(-1, get_state_of_line(hierarchy, 1, 0, address));
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 2, 0, address));
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 0, 0, address + 32));
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 1, 0, address + 32));
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 2, 0, address + 32));
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 0, 0, address + 64));
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 1, 0, address + 64));
    _assertEquals(CACHELINE_STATE_SHARED, get_state_of_line(hierarchy, 2, 0, address + 64));

    return 0;
}




int test_hierarchy(int argc, char **argv) {
    int failed_tests = 0;
    _test(test_init_cache_level, "test_init_cache_level");
    _test(test_write_on_other_cpu_invalidates, "test_write_on_other_cpu_invalidates");
    _test(test_read_on_other_cpu_stays_shared, "test_read_on_other_cpu_stays_shared");
    _test(test_higher_level_should_invalidate_lower_level_cache_on_evict,"test_higher_level_should_invalidate_lower_level_cache_on_evict");
    _test(test_multilevel, "test_multilevel");
    _test(test_multilevel_with_llc, "test_multilevel_with_llc");

    if(failed_tests == 0 ){
        printf("All tests passed!\n");
    } else {
        printf("One of the tests failed\n");
    }
    return 0;

}


