#include "multilevel.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "test.h"


int test_setup_single_level_single_cpu_cache() {
    struct MultiLevelCacheState* state = init_cachestate(1,1);
    state->states[0][0] = 
}


int main(int argc, char **argv) {
    int failed_tests = 0
    int successfull_tests = 0;
    _test(test_setup_multilevelcache, "test_setup_single_level_cache");
}
