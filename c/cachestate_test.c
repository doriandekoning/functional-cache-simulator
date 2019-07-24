#include "cachestate.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int tests_run = 0;

#define FAIL() printf("\n Failure in %s() line %d\n", __func__, __LINE__);
#define _assert(test) do {if (!(test)) { FAIL(); return 1; } } while(0)

#define _test(test, testname) do {int result = test(); failed_tests+= result; if(result) { printf("Failed %s!\n", testname);}else{printf("Passed %s\n", testname);}} while(0)
//;failed_tests+=result;if(result){printf("Test passed!\n"
//); } else {printf("Test %s failed!", "test");}} while(0)

int test_get_cache_set_number(){
    uint64_t expected = 0;
    _assert(get_cache_set_number(0) == 0);
    expected = 0;
    _assert(get_cache_set_number(AMOUNT_CACHE_SETS+10) == 10);
    return 0;
}

int test_get_cache_set_state() {
    CacheState state = calloc(AMOUNT_SIMULATED_PROCESSORS * CACHE_SIZE, sizeof(struct CacheEntry*));

    //Test when just getting the first line state
    struct CacheEntry firstEntry;
    state[0] = &firstEntry;
    _assert(get_cache_set_state(state, 0, 0) == &firstEntry);

    //Test when getting the line state from another cpu
    struct CacheEntry secondCPUEntry;
    state[AMOUNT_CACHE_SETS] = &secondCPUEntry;
    _assert(get_cache_set_state(state, 0, 1) == &secondCPUEntry);


    //Test getting the line state not from the first cacheline
    struct CacheEntry middleEntry;
    state[999] = &middleEntry;
    _assert(get_cache_set_state(state, 999, 0) == &middleEntry);

    //Test getting midle entry in another cpu
    struct CacheEntry middleOtherCPUEntry;
    state[999 + (2*AMOUNT_CACHE_SETS)] = &middleOtherCPUEntry;
    _assert(get_cache_set_state(state, 999, 2) == &middleOtherCPUEntry);

    return 0;
}

int test_get_cache_entry_state() {
    struct CacheEntry firstEntry = {.address = 1}; //TODO rename in CacheEntry to line (or official name)
    struct CacheEntry thirdEntry = {.address = 3};
    struct CacheEntry secondEntry = {.address = 2, .prev = &firstEntry, .next = &thirdEntry};
    firstEntry.next = &secondEntry;
    thirdEntry.prev = &secondEntry;

    //Test get first entry
    _assert(&firstEntry == get_cache_entry_state(&firstEntry, 1));
    //Test get second entry
    _assert(&secondEntry == get_cache_entry_state(&firstEntry, 2));
    //Test get third entry
    _assert(&thirdEntry == get_cache_entry_state(&firstEntry, 3));
    //Test not found
    _assert(NULL == get_cache_entry_state(&firstEntry, 81));
    return 0;
}

int test_set_cache_set_state() {
    CacheState state = calloc(AMOUNT_SIMULATED_PROCESSORS * CACHE_SIZE, sizeof(struct CacheEntry*));

    //Test set in empty location
    struct CacheEntry newEntry = {.address=10};
    set_cache_set_state(state, &newEntry, 10, 0);
    _assert(state[10]  == &newEntry);

    //Test override
    struct CacheEntry new10Entry = {.address=10};
    struct CacheEntry old10Entry = {.address=11};
    state[12] = &old10Entry;
    set_cache_set_state(state, &new10Entry, 12, 0);
    _assert(state[12]  == &new10Entry);

    //Test set in different cpu
     struct CacheEntry otherCPUEntry = {.address=10};
    set_cache_set_state(state, &newEntry, 10, 2);
    _assert(state[10 + (2 * AMOUNT_CACHE_SETS)] == &newEntry);

    return 0;
}

int test_remove_item() {
    struct CacheEntry next;
    struct CacheEntry prev;

    //Test removing item with only a next
    struct CacheEntry onlyNext = {.next = &next};
    next.prev = &onlyNext;
    remove_item(&onlyNext);
    _assert(NULL == next.prev);

    //Test removing item with only a next
    struct CacheEntry onlyPrev = {.prev = &prev};
    prev.next = &onlyPrev;
    remove_item(&onlyPrev);
    _assert(NULL == prev.next);


    //Test remove with prev and next
    struct CacheEntry both = {.prev = &prev, .next = &next};
    next.prev = &both;
    prev.next = &both;
    remove_item(&both);
    _assert(&next == prev.next);
    _assert(&prev == next.prev);
    return 0;
}

int test_append_item_empty_list() {
    struct CacheEntry new;
    _assert(append_item(NULL, &new) == &new);
    return 0;
}

int test_append_item() {
    struct CacheEntry first = {};
    struct CacheEntry second = {};
    CacheSetState result = append_item(&first, &second);
    _assert(result == &first);
    _assert(first.next == &second);
    _assert(second.prev = &first);

    return 0;
}

int test_append_item_long_list() {
    struct CacheEntry first = {};
    struct CacheEntry second = {};
    second.prev = &first;
    first.next = &second;
    struct CacheEntry third = {};
    third.prev = &second;
    second.next = &third;
    struct CacheEntry fourth = {};
    fourth.prev = &third;
    third.next = &fourth;

    struct CacheEntry new = {};
    CacheSetState result = append_item(&first, &new);
    _assert(result == &first);
    _assert(new.prev == &fourth);
    _assert(fourth.next = &new);

    return 0;
}

int test_move_first_item_back() {
    struct CacheEntry first = {};
    struct CacheEntry second = {};
    second.prev = &first;
    first.next = &second;
    struct CacheEntry third = {};
    third.prev = &second;
    second.next = &third;

    //Move first back
    move_item_back(&first);
    _assert(first.prev == &third);
    _assert(third.next == &first);
    _assert(second.prev == NULL);

    return 0;
}

int test_move_midle_item_back() {
    struct CacheEntry first = {};
    struct CacheEntry second = {};
    second.prev = &first;
    first.next = &second;
    struct CacheEntry third = {};
    third.prev = &second;
    second.next = &third;

    //Move first back
    move_item_back(&second);
    _assert(first.next == &third);
    _assert(third.prev == &first);
    _assert(second.prev == &third);
    _assert(first.prev == NULL);
    return 0;
}

int test_list_length_empty() {
    _assert(0 == list_length(NULL));
    return 0;
}

int test_list_length_single_item() {
    struct CacheEntry item = {};
    _assert(1 == list_length(&item));
    return 0;
}

int test_list_length_multiple() {
    struct CacheEntry first = {};
    struct CacheEntry second = {};
    second.prev = &first;
    first.next = &second;
    struct CacheEntry third = {};
    third.prev = &second;
    second.next = &third;
    struct CacheEntry fourth = {};
    fourth.prev = &third;
    third.next = &fourth;

    _assert(list_length(&first) == 4);
    _assert(list_length(&second) == 4);
    _assert(list_length(&third) == 4);
    _assert(list_length(&fourth) == 4);

    return 0;
}

int test_get_head_empty() {
    _assert(NULL == get_head(NULL));
    return 0;
}

int test_get_head_single() {
    struct CacheEntry entry = {};
    _assert(&entry == get_head(&entry));
    return 0;
}

int test_get_head() {
    struct CacheEntry first = {};
    struct CacheEntry second = {};
    second.prev = &first;
    first.next = &second;
    struct CacheEntry third = {};
    third.prev = &second;
    second.next = &third;
    struct CacheEntry fourth = {};
    fourth.prev = &third;
    third.next = &fourth;

    _assert(get_head(&first) == &first);
    _assert(get_head(&second) == &first);
    _assert(get_head(&third) == &first);
    _assert(get_head(&fourth) == &first);

    return 0;
}


CacheSetState setupCacheSetState() {
    struct CacheEntry* first = calloc(1, sizeof(struct CacheEntry));
    first->address = 1;
    struct CacheEntry* second = calloc(1, sizeof(struct CacheEntry));
    second->address = 2;
    second->prev = first;
    first->next = second;
    struct CacheEntry* third = calloc(1, sizeof(struct CacheEntry));
    third->address = 3;
    third->prev = second;
    second->next = third;
    struct CacheEntry* fourth = calloc(1, sizeof(struct CacheEntry));
    fourth->address = 4;
    fourth->prev = third;
    third->next = fourth;
    return first;
}

int test_apply_state_change_evict(){
    CacheSetState state = setupCacheSetState();
    get_head(state);
    struct statechange change = {.new_state = STATE_INVALID};
    struct CacheEntry* fourth = state->next->next->next;
    CacheSetState new_state = apply_state_change(state, state->next->next, change, 1);
    _assert(state->next->next == fourth);
    _assert(new_state == state);
    _assert(list_length(new_state) == 3);

    return 0;
}

int test_apply_state_change_evict_head(){
    CacheSetState state = setupCacheSetState();
    get_head(state);
    struct statechange change = {.new_state = STATE_INVALID};
    struct CacheEntry* second = state->next;
    CacheSetState new_state = apply_state_change(state, state, change, 0);
    _assert(new_state == second);
    _assert(list_length(new_state) == 3);
    return 0;
}

int test_apply_state_change_modified_third(){
    CacheSetState state = setupCacheSetState();
    get_head(state);
    struct statechange change = {.new_state = STATE_MODIFIED};
    CacheSetState new_state = apply_state_change(state, state->next->next, change, 0);
    _assert(new_state == state);
    _assert(list_length(new_state) == 4);
    _assert(state->next->next->address == 4);
    _assert(state->next->next->next->address == 3);
    _assert(state->next->next->next->state == STATE_MODIFIED);
    return 0;
}

int test_apply_state_change_modified_first(){
    CacheSetState state = setupCacheSetState();
    get_head(state);
    struct statechange change = {.new_state = STATE_MODIFIED};
    struct CacheEntry* second = state->next;
    CacheSetState new_state = apply_state_change(state, state, change, 0);
    _assert(new_state == second);
    _assert(list_length(new_state) == 4);
    _assert(new_state->next->next->address == 4);
    _assert(new_state->next->next->next->address == 1);
    _assert(new_state->next->next->next->state == STATE_MODIFIED);
    return 0;
}

int test_apply_state_insert(){
    CacheSetState state = setupCacheSetState();
    get_head(state);
    struct statechange change = {.new_state = STATE_MODIFIED};
    struct CacheEntry* second = state->next;
    CacheSetState new_state = apply_state_change(state, NULL, change, 9);
    _assert(state == new_state);
    _assert(list_length(new_state) == 5);
    _assert(state->next->next->next->next->state == STATE_MODIFIED);
    _assert(state->next->next->next->next->address == 9);

    return 0;
}



int test_apply_state_empty(){
    struct statechange change = {.new_state = STATE_MODIFIED};
    CacheSetState new_state = apply_state_change(NULL, NULL, change, 9);
    _assert(list_length(new_state) == 1);
    _assert(new_state->state == STATE_MODIFIED);
    _assert(new_state->address == 9);
    return 0;
}


int main(int argc, char **argv) {
    int failed_tests = 0;
    _test(test_get_cache_set_number, "test_get_cache_set_number");
    _test(test_get_cache_set_state, "test_get_cache_set_state");
    _test(test_get_cache_entry_state, "test_get_cache_entry_state");
    _test(test_set_cache_set_state, "test_set_cache_set_state");
    _test(test_remove_item, "test_remove_item");
    _test(test_append_item_empty_list, "test_append_item_empty_list");
    _test(test_append_item_long_list, "test_append_item_long_list");
    _test(test_append_item, "test_append_item");
    _test(test_move_first_item_back, "test_move_first_item_back");
    _test(test_list_length_empty, "test_list_length_empty");
    _test(test_list_length_single_item, "test_list_length_single_item");
    _test(test_list_length_multiple, "test_list_length_multiple");
    _test(test_get_head_empty, "test_get_head_empty");
    _test(test_get_head_single, "test_get_head_single");
    _test(test_get_head, "test_get_head");
    _test(test_apply_state_change_evict, "test_apply_state_change_evict");
    _test(test_apply_state_change_evict_head, "test_apply_state_change_evict_head");
    _test(test_apply_state_change_modified_third, "test_apply_state_change_modified_third");
    _test(test_apply_state_change_modified_first, "test_apply_state_change_modified_first");
    _test(test_apply_state_insert, "test_apply_state_insert");

    if(failed_tests == 0 ){
        printf("All tests passed!\n");
    } else {
        printf("One of the tests failed\n");
    }
    return 0;

}



