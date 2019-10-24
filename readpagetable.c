#include <stdio.h>

#include "pagetable.h"
#include "memory.h"



void print_table(int lvl, uint64_t* table) {
	for(int i = 0; i < 512; i++) {
		printf("L%d[%x]=0x%012lx\n", lvl, i, table[i]);
	}
}

int main(int argc, char** argv){
	if(argc < 4) {
		printf("First argument should be path!\n");
		return 1;
	}
	char* endptr = 0;
	char* endptr2 = 0;
	struct memory* mem = init_memory();
	uint64_t cr3 = strtol(argv[2], &endptr, 16);
	uint64_t virt = strtol(argv[3], &endptr2, 16);
	printf("CR3:\t%lx\n", cr3);
	printf("Virt:\t%lx\n", virt);
	if(read_pagetable(mem, argv[1])){
		printf("Pagetable could not be read:%s\n", argv[1]);
		return 0;
	}
	uint64_t read;
	read_sim_memory(mem, 0x2c01000, 8, &read);
//	printf("Printing pagetable:%d\n", print_pagetable(mem, cr3));

	printf("virt_to_phys:\t%lx\n", vaddr_to_phys(mem, cr3, virt));
return 0;
/*	uint64_t buf[512];
	read_sim_memory(mem, 0 & 0xFFFFFFFFF000ULL, 512*8, buf);
	print_table(0, buf);
	uint64_t l2 = buf[0xab];
	if(l2==0){printf("JEMOEDER!\n"); return 1;}
	read_sim_memory(mem, buf[0xab] & 0xFFFFFFFFF000ULL, 512*8, buf);
	print_table(1, buf);
	uint64_t l3 = buf[0xab];
	if(l3==NULL){printf("JEMOEDER2\n");return 1;}
	read_sim_memory(mem, buf[0xab] & 0xFFFFFFFFF000ULL, 512*8, buf);
	print_table(2, buf);
	uint64_t l4 = buf[0xab];
	if(l4==NULL){printf("JEMOEDER3\n");return 1;}
	read_sim_memory(mem, buf[0xab] & 0xFFFFFFFFF000ULL, 512*8, buf);
	print_table(3, buf);
*/	
	return 0;
}
