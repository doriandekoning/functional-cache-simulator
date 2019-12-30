#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "pagetable/pagetable.h"
#include "memory.h"
#include "config.h"

#define PG_PRESENT_MASK		(1UL << 0)
#define PG_PSE_MASK		(1UL << 7)
#define INDEX_MASK		0x1ffUL

uint64_t vaddr_to_phys(struct memory* mem, uint64_t l1_phys, uint64_t virtaddress, bool debug) {
	if(debug){
		printf("Cr3 in vaddrtophys:%lx\n", l1_phys);
	}
	uint64_t l1_index = (virtaddress >> 39) & INDEX_MASK;
	uint64_t l2_index = (virtaddress >> 30) & INDEX_MASK;
	uint64_t l3_index = (virtaddress >> 21) & INDEX_MASK;
	uint64_t l4_index = (virtaddress >> 12) & INDEX_MASK;
	//L1
	debug_printf("L1 Physical address:0x%012lx\n", l1_phys);
	//Find l2
	uint64_t l2_phys;
	debug_printf("Physaddr L1: %lx, with offset: %lx\n", l1_phys & 0x3fffffffff000ULL, (l1_phys & 0x3fffffffff000ULL)  +  (l1_index*8));
	if(debug){
		printf("Looking up l1_index:%lx, l1_phys:%lx at %lx\n", l1_index, l1_phys, (l1_phys & 0x3fffffffff000ULL)  +  (l1_index*8));
	}
	if(read_sim_memory(mem, (l1_phys & 0x3fffffffff000ULL) + (l1_index*8), 8, &l2_phys) != 8 || !(l2_phys & PG_PRESENT_MASK)) {
		debug_printf("Not found in l1 at: 0x%012lx\n",  (l1_phys & 0x3fffffffff000ULL) + (l1_index*8));
		// printf("notfound l1 %lx at %lx\n", l2_phys,  (l1_phys & 0x3fffffffff000ULL)  +  (l1_index*8));
		return NOTFOUND_ADDRESS; //Not found
	}
	if(debug) {
		printf("l1phys: %lx, l1_index: %lx\n", l1_phys, l1_index);
		printf("L1: %lx L2:%lx\n", (l1_phys & 0x3fffffffff000ULL)  +  (l1_index*8), l2_phys);
	}

	if(l2_phys & PG_PSE_MASK) {
		// 1G page found
		return  (l2_phys & 0xFFFFC0000000) | (virtaddress & 0x3FFFFFFF);
	}
	debug_printf("L2 Physical address:0x%012lx\n", l2_phys);


	//Find l3
	uint64_t l3_phys;
	debug_printf("Physaddr L2: %lx, with offset: %lx\n", l2_phys & 0x3fffffffff000ULL, (l2_phys & 0x3fffffffff000ULL)  +  (l2_index*8));
	if(read_sim_memory(mem, (l2_phys & 0x3fffffffff000ULL) + (l2_index*8), 8, &l3_phys) != 8 || !(l3_phys & PG_PRESENT_MASK)) {
		debug_printf("Not found in l2 at: 0x%012lx\n",  (l2_phys & 0x3fffffffff000ULL) + (l2_index*8));
		printf("notfound l2\n");
		return NOTFOUND_ADDRESS; //Not found
	}
	if(l3_phys & PG_PSE_MASK) {
		//2^30(1G) page
		return (l3_phys & 0xFFFFFFE00000) | (virtaddress & 0x1FFFFF);
	}
	debug_printf("L3 Physical address:0x%012lx\n", l3_phys);

	//Find l4
	uint64_t l4_phys;
	debug_printf("Physaddr L3: %lx, with offset: %lx\n", l3_phys & 0x3fffffffff000ULL, (l3_phys & 0x3fffffffff000ULL)  +  (l3_index*8));
	if(read_sim_memory(mem, (l3_phys & 0x3fffffffff000ULL) + (l3_index*8), 8, &l4_phys) != 8 || !(l4_phys & PG_PRESENT_MASK)){
		debug_printf("PGTABLE]l4_phys_raw: 0x%012lx\n", l4_phys);
		debug_printf("&PGPRESENTL:%lx\n", l4_phys & PG_PRESENT_MASK);
		debug_printf("Not found in l3 at: 0x%012lx\n",  (l3_phys & 0x3fffffffff000ULL) + (l3_index*8));
		printf("notfound l3\n");
		return NOTFOUND_ADDRESS; //Not found
	}

	if(l4_phys & PG_PSE_MASK) {
		//2^21(2M) page
		return (l4_phys & 0xFFFFFFF80000) | (virtaddress & 0x7FFFF);
	}
        //Find l4
        uint64_t phys;
	debug_printf("L4 Physical address:0x%012lx\n", l4_phys);
        if(read_sim_memory(mem, (l4_phys & 0x3fffffffff000ULL) + (l4_index*8), 8, &phys) != 8 || !(phys & PG_PRESENT_MASK)){
				printf("notfound l4\n");
                return NOTFOUND_ADDRESS; //Not found
        }
	return  (phys & 0x3fffffffff000ULL) | (virtaddress & 0xFFF);
}

int print_pagetable(struct memory* mem, uint64_t cr3) {
	uint64_t l1buf[512];
	uint64_t l2buf[512];
	uint64_t l3buf[512];
	uint64_t l4buf[512];
	if(read_sim_memory(mem, (cr3 & 0x3fffffffff000ULL), 8*512, l1buf) != 8*512) {
		return -1;
	}
	for(uint64_t l1 = 0; l1 < 512; l1++) {
		if(!(l1buf[l1] & PG_PRESENT_MASK)) {
			continue;
		}
		if(l1buf[l1] & PG_PSE_MASK) {
			continue;
		}
		//check PSE
		if(read_sim_memory(mem, (l1buf[l1] & 0x3fffffffff000ULL), 8*512, l2buf) != 8*512) {
			return -2;
		}
		for(uint64_t l2 = 0; l2 < 512; l2++) {
			if(!(l2buf[l2] & PG_PRESENT_MASK)) {
				continue;
			}
			if(l2buf[l2] & PG_PSE_MASK) {
				continue;
			}
			if(read_sim_memory(mem, (l2buf[l2] & 0x3fffffffff000ULL), 8*512, l3buf) != 8*512) {
				return -3;
			}
			for(uint64_t l3 = 0; l3 < 512; l3++) {
				if(l3 == 0) {
					printf("L3: 0x%012lx\n", l3buf[l3]);
				}
				if(!(l3buf[l3] & PG_PRESENT_MASK)) {
					continue;
				}
				if(read_sim_memory(mem, (l3buf[l3] & 0x3fffffffff000ULL), 8*512, l4buf) != 8*512) {
					return -3;
				}
				for(uint64_t l4 = 0; l4 < 512; l4++) {
					if(l4buf[l4] & PG_PRESENT_MASK) {
						printf("0x%012lx -- 0x%012lx\n", (l1 <<  39) | (l2 << 30) | (l3 << 21) | (l4<<12), l4buf[l4]);
					}
				}
			}
		}
	}
}

int read_pagetable(struct memory* mem, char* path) {
	FILE* f = fopen(path, "r");
	uint64_t cr3;
	uint64_t totalLevelsRead = 0;
	if(!f) {
		return 1;
	}
	uint64_t physaddr;
	uint64_t buf[1<<12];
	uint64_t offset = 0;
	bool first = true;
	uint64_t stored_at;
	uint64_t stored_at_val;
	while(true){
		if(fread(&physaddr, sizeof(uint64_t), 1, f) != 1) {
			printf("Could not read physaddr\n");
			//TODO verify EOF
			break;
		}
		if(first){
			printf("Read CR3: 0x%012lx\n", physaddr);
		}
		int size = 512;
		if(fread(buf, sizeof(uint64_t), size, f) != size) {
			printf("Could not read pagetable\n");
			return 1;
		}
#ifdef DEBUG
/*			for(int i = 0; i < 512; i++) {
				printf("Read[%x]=0x%012lx storing at: 0x%012lx\n", i, buf[i], (8*i)+(physaddr&0x3fffffffff000ULL));
			}*/
#endif
		write_sim_memory(mem, physaddr & 0x3fffffffff000ULL, size*8, buf);
		totalLevelsRead++;
		first = false;
	}
	printf("Total subtables:%d\n", totalLevelsRead);
	return 0;

}
