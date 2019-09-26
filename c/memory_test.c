#include "memory.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "test.h"


int tests_run = 0;


int test_init() {
	struct memory* mem = init_memory();

	return 0;
}

int test_write() {
	struct memory* mem = init_memory();
	uint64_t value = 54321;
	int written = write(mem, 12334, 8, (uint8_t*)&value);
	_assertEquals(8, written);
	return 0;
}

int test_read() {
	struct memory* mem = init_memory();
	uint64_t value = 54321;
	int written = write(mem, 1234, 8, (uint8_t*)&value);
	_assertEquals(8, written);
	uint64_t read_val = 0;
	int read_bytes = read(mem, 1234, 8, (uint8_t*)&read_val);
	_assertEquals(8, read_bytes);
	_assertEquals(value, read_val);
	return 0;
}

int test_overwrite() {
	struct memory* mem = init_memory();
	uint64_t value = 54321;
	int written = write(mem, 1234, 8, (uint8_t*)&value);
	_assertEquals(8, written);
	uint64_t value2 = 99999999;
	written = write(mem, 1234, 8, (uint8_t*)&value2);
	uint64_t read_val = 0;
	int read_bytes = read(mem, 1234, 8, (uint8_t*)&read_val);
	_assertEquals(8, read_bytes);
	_assertEquals(value2, read_val);
	return 0;
}



int test_read_empty() {
	struct memory* mem = init_memory();
	int64_t value = 0;
	int read_bytes = read(mem, 54321, 8, (uint8_t*)value);
	_assertEquals(read_bytes, 0);
	return 0;
}

int main(int argc, char **argv) {
	int failed_tests = 0;
	_test(test_init, "test_init");
	_test(test_write, "test_write");
	_test(test_read, "test_read");
	_test(test_read_empty, "test_read");
	_test(test_overwrite, "test_overwrite");
	if(failed_tests == 0 ){
		printf("All tests passed!\n");
	} else {
		printf("One of the tests failed\n");
	}
	return 0;
}
