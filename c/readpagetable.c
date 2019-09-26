#include <stdio.h>

#include "pagetable.h"



int main(int argc, char** argv){
	if(argc < 2) {
		printf("First argument should be path!\n");
		return 1;
	}
	uint64_t cr3;
	pagetable* pgtable = read_pagetable(argv[1], &cr3);
	if(pgtable == NULL){
		printf("Pagetable could not be read:%s\n", argv[1]);
		return 0;
	}
	printf("CR3:\t%lx\n", cr3);
	printf("Virt:\t%lx\n", 0xffffffffbee00000);
	printf("Phys:\t%lx\n", vaddr_to_phys(pgtable, 0xffffffffbee00000));
	return 0;
}
