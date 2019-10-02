#include "memory.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "test.h"


int testos_run = 0;


int test_init() {
	struct memory* mem = init_memory();

	return 0;
}

int test_write() {
	struct memory* mem = init_memory();
	uint64_t value = 54321;
	int written = write_sim_memory(mem, 0xAFF, 8, (uint8_t*)&value);
	_assertEquals(8, written);
	return 0;
}


int test_write_border() {
        struct memory* mem = init_memory();
        uint32_t value = 0x12;
        int written = write_sim_memory(mem, 0xffffffffff5fc0b0, 4, (uint8_t*)&value);
        _assertEquals(4, written);
        return 0;
}

int test_read_border() {
        struct memory* mem = init_memory();
	uint64_t addr = 0xffffffffff5fc0b0;
        uint32_t value = 0x12;
        int written = write_sim_memory(mem, addr, 4, (uint8_t*)&value);
        _assertEquals(4, written);
	uint32_t read_value;
	int bytes_read = read_sim_memory(mem, addr, 4, (uint8_t*)&read_value);
	_assertEquals(bytes_read, 4);
	_assertEquals(value, read_value);
        return 0;
}

int test_read() {
	struct memory* mem = init_memory();
	uint64_t value = 54321;
	int written = write_sim_memory(mem, 1234, 8, (uint8_t*)&value);
	_assertEquals(8, written);
	uint64_t read_val = 0;
	int read_bytes = read_sim_memory(mem, 1234, 8, (uint8_t*)&read_val);
	_assertEquals(8, read_bytes);
	_assertEquals(value, read_val);
	return 0;
}

int test_overwrite() {
	struct memory* mem = init_memory();
	uint64_t value = 54321;
	int written = write_sim_memory(mem, 1234, 8, (uint8_t*)&value);
	_assertEquals(8, written);
	uint64_t value2 = 99999999;
	written = write_sim_memory(mem, 1234, 8, (uint8_t*)&value2);
	uint64_t read_val = 0;
	int read_bytes = read_sim_memory(mem, 1234, 8, (uint8_t*)&read_val);
	_assertEquals(8, read_bytes);
	_assertEquals(value2, read_val);
	return 0;
}



int test_read_empty() {
	struct memory* mem = init_memory();
	int64_t value = 0;
	int read_bytes = read_sim_memory(mem, 54321, 8, (uint8_t*)value);
	_assertEquals(read_bytes, 0);
	return 0;
}

int main(int argc, char **argv) {
	int failed_tests = 0;
	_test(test_init, "test_init");
	_test(test_write, "test_write");
	_test(test_read, "test_read");
	_test(test_read_empty, "test_read");
	_test(test_write_border, "test_write_border");
	_test(test_overwrite, "test_overwrite");
	_test(test_read_border, "test_read_border");
	if(failed_tests == 0 ){
		printf("All tests passed!\n");
	} else {
		printf("One of the tests failed\n");
	}
	return 0;
}
