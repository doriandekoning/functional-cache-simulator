#ifndef __TEST_H
#define __TEST_H

#

#define FAIL() printf("\n Failure in %s() line %d\n", __func__, __LINE__);
#define _assert(test) do {if (!(test)) { FAIL(); return 1; } } while(0)
#define _assertEquals(expected, actual) do {if(expected != actual) { printf("Expected was: %llu but actual was: %llu at line %d\n", (uint64_t)expected, (uint64_t)actual, __LINE__); return 1;}} while(0)

#define _test(test, testname) do {int result = test(); failed_tests+= result; if(result) { printf("Failed %s!\n", testname);}else{printf("Passed %s\n", testname);}} while(0)



#endif /* #ifndef __TEST_H */
