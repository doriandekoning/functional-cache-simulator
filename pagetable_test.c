#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "test.h"
#include "memory.h"
#include "pagetable.h"


int tests_run = 0;

int test_map_single_level() {
	const uint64_t address = 1234;
	struct memory* mem  = init_memory();
	_assertEquals((1 << 12), vaddr_to_phys(&table, address));
	_assertEquals(( 1 << 12), vaddr_to_phys(&table, address)); // Check address is mapped to same value the second time
	_assertEquals((1 << 12), (uint64_t)(table.root[address >> 12].next));
	return 0;
}

int test_map_single_level_two_entries() {
	const uint64_t address1 = (uint64_t)1234 << 20;
	const uint64_t address2 = (uint64_t)4321 << 20;
	nextfreephys = (1<<12);
	struct memory* mem = init_memory()
	_assertEquals((1<<12), vaddr_to_phys(mem, &table, address1));
	_assertEquals(2*(1<<12), vaddr_to_phys(mem, &table, address2)); // Check address is mapped to same value the second time

	return 0;
}

/*int test_map_two_levels_single_entry() {
	struct memory* mem = init_memory();
	nextfreephys = (1 << 12);
	const uint64_t address1 = (1234) + (2 << 18);
	_assertEquals((1 << 12), vaddr_to_phys(&table, address1));
	return 0;
}

int test_map_two_levels_single_two_entry() {
	pagetable table = {.levels = 2, .addressLength = 32};
	nextfreephys = (1 << 12);
	const uint64_t address1 = (1234) + (2 << 18);
	const uint64_t address2 = (4321) + (2 << 18);
	_assertEquals((1 << 12), vaddr_to_phys(&table, address1));
	_assertEquals(2*(1 << 12), vaddr_to_phys(&table, address2));
	_assertEquals((1 << 12), vaddr_to_phys(&table, address1));
	_assertEquals(2*(1 << 12), vaddr_to_phys(&table, address2));
	return 0;
}

int test_map_four_levels_two_entries() {
	pagetable table = {.levels = 4, .addressLength = 32};
	nextfreephys = (1 << 12);
	const uint64_t address1 = (1234) + (2 << 18);
	const uint64_t address2 =  (((uint64_t)4321 << 16) + ((uint64_t)1221 << 8)  + ((uint64_t)1234)) << 12;
	_assertEquals((1 << 12), vaddr_to_phys(&table, address1));
	_assertEquals(2*(1 << 12), vaddr_to_phys(&table, address2));
	_assertEquals((1 << 12), vaddr_to_phys(&table, address1));
	_assertEquals(2*(1 << 12), vaddr_to_phys(&table, address2));
	return 0;
}

int test_multilevel() {
	pagetable table = {.levels = 3, .addressLength = 54};
	const uint64_t addr = (uint64_t)18446741874686299840;
	printf("%lx, %lx, %lx, %lx\n", addr, addr  >> 12, (addr >> 12) % (1<<18), (addr >> 30) % ( 1 << 18));
	_assertEquals( (1 << 12), vaddr_to_phys(&table, addr));
	return 0;
}

int test_get_level_tag() {
	pagetable table = {.levels = 2, .addressLength = 32};
	const uint64_t address = (((uint64_t)4321 << 16)  + ((uint64_t)1234)) << 12;
	_assertEquals(1234, get_level_tag(&table, address, 0));
	_assertEquals(4321, get_level_tag(&table, address, 1));
	return 0;
}
*/


int main(int argc, char **argv) {
	int failed_tests = 0;
	_test(test_bits_per_level, "test_bits_per_level");
	_test(test_bits_per_level_not_divisable, "test_bits_per_level_not_divisable");
	_test(test_pages_per_level, "test_pages_per_level");
	_test(test_map_single_level, "test_map_single_level");
	_test(test_pages_per_level_single_level, "test_pages_per_level_single_level");
	_test(test_map_single_level_two_entries, "test_map_single_level_two_entries");
	_test(test_map_two_levels_single_entry, "test_map_two_levels_single_entry");
	_test(test_get_level_tag, "test_get_level_tag");
	_test(test_map_two_levels_single_two_entry, "test_map_two_levels_single_two_entry");
	_test(test_map_four_levels_two_entries, "test_map_four_levels_two_entries");
	_test(test_multilevel, "test_multilevel");
	if(failed_tests == 0 ){
		printf("All tests passed!\n");
	} else {
		printf("One of the tests failed\n");
	}
	return 0;
}
