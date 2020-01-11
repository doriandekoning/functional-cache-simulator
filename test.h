#ifndef __TEST_H
#define __TEST_H

#

#define FAIL() printf("\n\033[1;31m Failure in %s() line %d\033[0m\n", __func__, __LINE__);
#define _assert(test) do {if (!(test)) { FAIL(); return 1; } } while(0)
#define _assertEquals(expected, actual) do {if(expected != actual) { printf("\033[1;31mExpected was: 0x%llx but actual was: 0x%llx at line %d\033[0m\n", (uint64_t)expected, (uint64_t)actual, __LINE__); return 1;}} while(0)

#define _test(test, testname) do {printf("Running: %s\n", testname); int result = test(); failed_tests+= result; if(result) { printf("Failed %s!\n", testname);}else{printf("\033[0;32mPassed %s\033[0m\n", testname);}} while(0)



#endif /* #ifndef __TEST_H */
