#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <byteswap.h>

#include "test.h"
#include "memory.h"


int testos_run = 0;

FILE* test_backing_file;


int test_init() {
	struct Memory* mem = init_memory(NULL);

	return 0;
}

int test_write_last_level() {
	struct Memory* mem = init_memory(NULL);
	uint64_t value = 0x654321;
	int written = write_sim_memory(mem, 0xAFF, 8, (uint8_t*)&value);
	_assertEquals(8, written);
	struct Memory* l1 = mem->table[0];
	if(l1 == NULL){ return 1; }
	struct Memory* l2 = l1->table[0];
	if(l2 == NULL){return 1;}
	struct Memory* l3 = l2->table[0];
	if(l3 == NULL) {return 1;}
	struct Memory* l4 = l3->table[0];
	if(l4 == NULL) {return 1;}
	struct Memory* l5 = l4->table[0];
	if(l5 == NULL) {return 1;}
	uint64_t result;
	printf("Table is at:%p\n", ((void*)l5->table));
	printf("Test is reading from:%p\n", ((void*)l5->table)+0xAFF);
	memcpy(&result, ((void*)l5->table)+0xAFF, 8);
	_assertEquals(value, result);
	return 0;
}

int test_write_not_zero() {
	struct Memory* mem = init_memory(NULL);
	uint64_t value = 888888;
	uint64_t addr = 0x111CCCABA123;
	int written = write_sim_memory(mem, addr, 8, (uint8_t*)&value);
	_assertEquals(8, written);
	uint64_t read_value;
	if(8 != read_sim_memory(mem, addr, 8, &read_value)) {
		printf("unable to read sim memory!\n");
		return 1;
	}
	_assertEquals(value, read_value);
	struct Memory* l1 = mem->table[((addr >> 47) & 0x1)];
	if(l1 == NULL) { return 1;}
	struct Memory* l2 = l1->table[((addr >> 39) & 0x1FF)];
	if(l2 == NULL) {return 1;}
	struct Memory* l3 = l2->table[((addr >> 30) & 0x1FF)];
	if(l3 == NULL) { return 1;}
	struct Memory* l4 = l3->table[((addr >> 21) & 0x1FF)];
	if(l4 == NULL) { return 1;}
	struct Memory* l5 = l4->table[((addr >> 12) & 0x1FF)];
	if(l5 == NULL) { return 1;}
	uint64_t result = 0;
	memcpy(&result, ((uint8_t*)l5->table) + (addr & 0xFFF), 8);
	_assertEquals(value, result);
	return 0;
}

int test_write_border() {
        struct Memory* mem = init_memory(NULL);
        uint64_t value = (uint64_t)-1;
        int written = write_sim_memory(mem, 0xffffffff0fffff-4, 8, (uint8_t*)&value);
        _assertEquals(8, written);
        return 0;
}

int test_write_read_somewhere(){
	struct Memory* mem = init_memory(NULL);
	uint64_t value = 123454321;
	_assertEquals(8, write_sim_memory(mem, 0x32fc2a888, 8, &value));
	uint64_t value_read;
	_assertEquals(8, read_sim_memory(mem, 0x32fc2a888, 8, &value_read));
	_assertEquals(value, value_read);
	return 0;
}

int test_read_border() {
	struct Memory* mem = init_memory(NULL);
	uint64_t addr = 0xfffff5fc0b0;
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
	struct Memory* mem = init_memory(NULL);
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
	struct Memory* mem = init_memory(NULL);
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




int test_read_write_full_page() {
	struct Memory* mem = init_memory(NULL);
	int64_t values[512];
	for(int i = 0; i < 512; i++) {
		values[i] = 0xFFF-i;
	}
	int written_bytes = write_sim_memory(mem, 0xabc000, 0xFFF, values);
	_assertEquals(written_bytes, 0xFFF);
	uint64_t read_values[512];
	int read_bytes = read_sim_memory(mem, 0xabc000, 0xFFF, read_values);
	for(int i = 0; i < 512; i++) {
		_assertEquals(read_values[i], 0xFFF-i);
	}
	return 0;
}

int test_rw_uint64() {
	struct Memory* mem = init_memory(NULL);
	uint32_t val = 0x12300;
	uint64_t val_extended = (0xFFFFFFFF00000000 | val);
	uint64_t addr = 0x123345678;
	printf("Val:\t0x%012lx\n", val);
	printf("Val_ext:\t0x%012lx\n", val_extended);
	_assertEquals(4, write_sim_memory(mem, addr, 4, &val_extended));
	uint64_t val_read;
	_assertEquals(4, read_sim_memory(mem, addr, 4, &val_read));
	_assertEquals(val_read, val);
	_assertEquals(val, (uint32_t)val_read);
	printf("Val read:\t0x%012lx\n", val_read);
	return 0;
}

int test_write_full_read_partial_page() {
        struct Memory* mem = init_memory(NULL);
        int64_t values[512];
        for(int i = 0; i < 512; i++) {
                values[i] = 0xFFF-i;
        }
        int written_bytes = write_sim_memory(mem, 0x00032fc2a000, 0xFFF, values);
        _assertEquals(written_bytes, 0xFFF);
        for(int i = 0; i < 512; i++) {
			uint64_t read_value;
			read_sim_memory(mem, 0x00032fc2a000 + (8*i), 8, &read_value);
			_assertEquals(values[i], read_value);
        }
        return 0;
}


int test_error_in_sim() {
	struct Memory* mem = init_memory(NULL);
	uint32_t value = 0xbffbfeac;
	uint64_t address = 0xbffacfff;
	_assertEquals(4, write_sim_memory(mem, address, 4, &value));

	uint32_t read_val;
	_assertEquals(4, read_sim_memory(mem, address, 4, &read_val));
	_assertEquals(value, read_val);
	return 0;
}

int test_big_little_endian() {
	struct Memory* mem = init_memory(NULL);
	uint8_t value[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
	uint64_t address = 0xbffac000;
	_assertEquals(8, write_sim_memory(mem, address, 8, &value));

	uint8_t read_value[] = {0x00, 0x00, 0x00, 0x00};
	_assertEquals(4, read_sim_memory(mem, address, 4, &read_value));
	printf("%x, %x, %x, %x\n", read_value[0], read_value[1], read_value[2], read_value[3]);
	return 0;
}

int test_read_int32() {
	struct Memory* mem = init_memory(NULL);
	uint64_t value = 0x123456789abcdeff;
	uint64_t address =  0x2aff8;
	_assertEquals(4, write_sim_memory(mem, address, 4, &value));
	uint32_t read_value = 0;
	_assertEquals(4, read_sim_memory(mem, address, 4, &read_value));
	_assertEquals(read_value, 0x9abcdeff);

	return 0;
}

int test_backing_memory() {
	FILE* test_backing_file = fopen("memory/testbackingfile", "r");
	if(test_backing_file == NULL) {
		printf("Unable to open test backing file!\n");
		return 1;
	}
	struct Memory* mem = init_memory(test_backing_file);
	int cur_addr = 0;
	size_t size_read;
	uint8_t read_val;
	uint8_t val_in_file;
	while(1) {
		fseek(mem->backing_file, cur_addr, SEEK_SET);
		size_read = fread(&val_in_file, 1, 1, test_backing_file);
		if(size_read != 1) break;
		_assertEquals(1, read_sim_memory(mem, cur_addr, 1, &read_val));
		_assertEquals(read_val, val_in_file);
		cur_addr++;
	};
	fclose(test_backing_file);
	return 0;
}

int test_backing_memory_write() {
	FILE* test_backing_file = fopen("memory/testbackingfile", "r");
	if(test_backing_file == NULL) {
		printf("Unable to open test backing file!\n");
		return 1;
	}
	struct Memory* mem = init_memory(test_backing_file);
	uint64_t addr = 0x1234;
	uint8_t file_val, mem_val;
	fseek(mem->backing_file, addr, SEEK_SET);
	_assertEquals(1, fread(&file_val, 1, 1, test_backing_file));
	_assertEquals(1, read_sim_memory(mem, addr, 1, &mem_val));
	_assertEquals(file_val, mem_val);
	uint8_t new_val = ~file_val;
	_assertEquals(1, write_sim_memory(mem, addr, 1, &new_val));
	_assertEquals(1, read_sim_memory(mem, addr, 1, &mem_val));
	_assertEquals((uint8_t)~file_val, mem_val);
	fclose(test_backing_file);
	return 0;
}



int main(int argc, char **argv) {
	int failed_tests = 0;

	_test(test_init, "test_init");
	_test(test_write_last_level, "test_write_last_level");
	_test(test_write_not_zero, "test_write_not_zero");
	_test(test_read, "test_read");
	_test(test_write_border, "test_write_border");
	_test(test_overwrite, "test_overwrite");
	_test(test_read_border, "test_read_border");
	_test(test_read_write_full_page, "test_read_write_full_page");
	_test(test_write_read_somewhere, "test_write_read_somewhere");
	_test(test_write_full_read_partial_page, "test_write_full_read_partial_page");
	_test(test_rw_uint64, "test_rw_uint64");
	_test(test_error_in_sim, "test_error_in_sim");
	_test(test_big_little_endian, "test_big_little_endian");
	_test(test_read_int32, "test_read_int32");
	_test(test_backing_memory, "test_backing_memory");
	_test(test_backing_memory_write, "test_backing_memory_write");
	if(failed_tests == 0 ){
		printf("All tests passed!\n");
	} else {
		printf("One of the tests failed\n");
	}
	return 0;
}
