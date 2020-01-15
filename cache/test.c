#ifdef TEST
#include "state_test.h"
#include "hierarchy_test.h"

int test_cases_run = 0;

#ifndef NOT_TEST_ALL
#define TEST_STATE
#define TEST_HIERARCHY
#endif


int main() {
    #ifdef TEST_STATE
    test_state();
    #endif
    #ifdef TEST_HIERARCHY
    test_hierarchy();
    #endif
}
#endif
