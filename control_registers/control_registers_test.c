#ifdef TEST

#include "control_registers.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "test.h"


int test_set_and_get_single_cpu() {
    ControlRegisterValues vals = init_control_register_values(1);
    uint64_t val = 0xabcf;
    set_cr_value(vals, 0, 3, val);
    _assertEquals(val, get_cr_value(vals, 0, 3));
    return 0;
}


int test_set_and_get_twice_single_cpu() {
    ControlRegisterValues vals = init_control_register_values(1);
    set_cr_value(vals, 0, 3, 0x1234);
    uint64_t val = 0xabcf;
    set_cr_value(vals, 0, 3, val);
    _assertEquals(val, get_cr_value(vals, 0, 3));
    return 0;
}

int test_set_and_get_multiple_cpu() {
    ControlRegisterValues vals = init_control_register_values(4);
    uint64_t val_cpu0 = 0x1234;
    uint64_t val_cpu2 = 0xffff;
    set_cr_value(vals, 0, 3, val_cpu0);
    set_cr_value(vals, 2, 3, val_cpu2);
    _assertEquals(val_cpu0, get_cr_value(vals, 0, 3));
    _assertEquals(val_cpu2, get_cr_value(vals, 2, 3));
    return 0;
}


int main(int argc, char **argv) {
    int failed_tests = 0;
    int successfull_tests = 0;
    _test(test_set_and_get_single_cpu, "test_set_and_get_single_cpu");
    _test(test_set_and_get_twice_single_cpu, "test_set_and_get_twice_single_cpu");
    _test(test_set_and_get_multiple_cpu, "test_set_and_get_multiple_cpu");
}


#endif
