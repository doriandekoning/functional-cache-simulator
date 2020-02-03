#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "pagetable/pagetable.h"
#include "memory/memory.h"
#include "config.h"

#define PG_PRESENT_MASK		(1UL << 0)
#define PG_PSE_MASK		(1UL << 7)
#define INDEX_MASK		0x1ffUL
#define PG_USER_MASK     (1 << 2)
#define PG_CACHE_DISABLE_MASK ( 1 << 4)


uint32_t vaddr_to_phys32(int amount_mems, struct Memory* mems, uint32_t cr3_value, uint32_t virtaddress, bool debug) {
	uint64_t l1_index = (virtaddress >> 22) & 0x3ff;
	uint64_t l2_index = (virtaddress >> 12) & 0x3ff;

	uint32_t pde = cr3_value & ~0xfff;
	uint32_t pte;
	if(read_sim_memory(amount_mems, mems, ((pde & ~0xfff) + (l1_index*4)), 4, &pde) != 4 || !(pde & PG_PRESENT_MASK)) {
		printf("Not found in pde\n");
		return (uint32_t)NOTFOUND_ADDRESS;
	}
	//TODO check if PG_PSE_MASK is set

	if(debug) printf("L2 Physical address: %x\n", pde);

	if(read_sim_memory(amount_mems, mems, (pde & ~0xfff) + (l2_index*4), 4, &pte) != 4 || !(pte & PG_PRESENT_MASK)) {
		printf("Not found in pte\n");
		return (uint32_t)NOTFOUND_ADDRESS;
	}

	return (pte & ~0xfff) | (virtaddress & 0xfff);

}

//TODO remove debug parameter
uint64_t vaddr_to_phys(int amount_mems, struct Memory* mems, uint64_t l1_phys, uint64_t virtaddress, bool* is_user_page, bool* cache_disable) {
	debug_printf("Cr3 in vaddrtophys:%lx\n", l1_phys);
	uint64_t l1_index = (virtaddress >> 39) & INDEX_MASK;
	uint64_t l2_index = (virtaddress >> 30) & INDEX_MASK;
	uint64_t l3_index = (virtaddress >> 21) & INDEX_MASK;
	uint64_t l4_index = (virtaddress >> 12) & INDEX_MASK;
	*is_user_page = true;
	//L1
	uint64_t l2_phys;
	debug_printf("Physaddr L1: %lx, with offset: %lx\n", l1_phys & 0x3fffffffff000ULL, (l1_phys & 0x3fffffffff000ULL)  +  (l1_index*8));
	debug_printf("Looking up l1_index:%lx, l1_phys:%lx at %lx\n", l1_index, l1_phys, (l1_phys & 0x3fffffffff000ULL)  +  (l1_index*8));

	// Find L2
	if(read_sim_memory(amount_mems, mems, (l1_phys & 0x3fffffffff000ULL) + (l1_index*8), 8, &l2_phys) != 8 || !(l2_phys & PG_PRESENT_MASK)) {//TODO fix l1phys
		printf("Not found in l1 at: 0x%012llx, L2_phys:%lx, cr3: %lx\n",  (l1_phys & 0x3fffffffff000ULL) + (l1_index*8), l2_phys, l1_phys);
		// printf("notfound l1 %lx at %lx\n", l2_phys,  (l1_phys & 0x3fffffffff000ULL)  +  (l1_index*8));
		return NOTFOUND_ADDRESS; //Not found
	}
	debug_printf("l1phys: %lx, l1_index: %lx\n", l1_phys, l1_index);
	debug_printf("L1: %lx L2:%lx\n", (l1_phys & 0x3fffffffff000ULL)  +  (l1_index*8), l2_phys);
	l2_phys &= ~(1UL << 63);
	if(*is_user_page && !(l2_phys & PG_USER_MASK)) {
		*is_user_page = false;
	}
	if(l2_phys & PG_PSE_MASK) {
		// 1G page found
		return  (l2_phys & 0xFFFFC0000000) | (virtaddress & 0x3FFFFFFF);
	}
	debug_printf("L2 Physical address:0x%012lx\n", l2_phys);


	//Find l3
	uint64_t l3_phys;
	debug_printf("Physaddr L2: %llx, with offset: %lx\n", l2_phys & 0x3fffffffff000ULL, (l2_phys & 0x3fffffffff000ULL)  +  (l2_index*8));
	if(read_sim_memory(amount_mems, mems, (l2_phys & 0x3fffffffff000ULL) + (l2_index*8), 8, &l3_phys) != 8 || !(l3_phys & PG_PRESENT_MASK)) {
		printf("Not found in l2 at: 0x%012llx\n",  (l2_phys & 0x3fffffffff000ULL) + (l2_index*8));
		return NOTFOUND_ADDRESS; //Not found
	}
	l3_phys &= ~(1UL << 63);

	if(*is_user_page && !(l3_phys & PG_USER_MASK)) {
		*is_user_page = false;
	}

	if(l3_phys & PG_PSE_MASK) {
		//2^30(1G) page
		debug_print("Found 1GB page!\n");
		*cache_disable = !!(l3_phys & PG_CACHE_DISABLE_MASK);
		return (l3_phys & ~(((0x1UL) << 30) -1)) | (virtaddress & (((0x1UL) << 30) -1));
	}

	debug_printf("L3 Physical address:0x%012lx\n", l3_phys);

	//Find l4
	uint64_t l4_phys;
	debug_printf("Physaddr L3: %lx, with offset: %lx\n", l3_phys & 0x3fffffffff000ULL, (l3_phys & 0x3fffffffff000ULL)  +  (l3_index*8));
	if(read_sim_memory(amount_mems, mems, (l3_phys & 0x3fffffffff000ULL) + (l3_index*8), 8, &l4_phys) != 8 || !(l4_phys & PG_PRESENT_MASK)){
		debug_printf("PGTABLE]l4_phys_raw: 0x%012llx\n", l4_phys);
		debug_printf("&PGPRESENTL:%lx\n", l4_phys & PG_PRESENT_MASK);
		printf("Not found in l3 at: 0x%012llx\n",  (l3_phys & 0x3fffffffff000ULL) + (l3_index*8));
		return NOTFOUND_ADDRESS; //Not found
	}
	l4_phys &= ~(1UL << 63);

	if(*is_user_page && !(l4_phys & PG_USER_MASK)) {
		*is_user_page = false;
	}

	if(l4_phys & PG_PSE_MASK) {
		//2^21(2M) page
		*cache_disable = !!(l4_phys & PG_CACHE_DISABLE_MASK);
	// printf("tlbe:%lx\n", l4_phys);

		debug_printf("Found 2M page! entry: %lx from virt: %lx at %lx\n", l4_phys, virtaddress, l3_phys);
		return (l4_phys & ~(((0x1UL) << 21) -1)) | (virtaddress & (((0x1UL) << 21) -1));
	}
	//Find l4

	uint64_t phys;
	debug_printf("L4 Physical address:0x%012lx\n", l4_phys);
	if(read_sim_memory(amount_mems, mems, (l4_phys & 0x3fffffffff000ULL) + (l4_index*8), 8, &phys) != 8 || !(phys & PG_PRESENT_MASK)){
		printf("notfound l4 entry physaddr:%llx\n", (l4_phys & 0x3fffffffff000ULL) + (l4_index*8));
		return NOTFOUND_ADDRESS; //Not found
	}
	// printf("tlbe:%lx\n", phys);
	if(*is_user_page && !(phys & PG_USER_MASK)) {
		*is_user_page = false;
	}
	*cache_disable = !!(phys & PG_CACHE_DISABLE_MASK);

	return  (phys & 0x3fffffffff000ULL) | (virtaddress & 0xFFFUL);
}

int print_pagetable(int amount_mems, struct Memory* mems, uint64_t cr3) {
	uint64_t l1buf[512];
	uint64_t l2buf[512];
	uint64_t l3buf[512];
	uint64_t l4buf[512];
	if(read_sim_memory(amount_mems, mems, (cr3 & 0x3fffffffff000ULL), 8*512, l1buf) != 8*512) {
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
		if(read_sim_memory(amount_mems, mems, (l1buf[l1] & 0x3fffffffff000ULL), 8*512, l2buf) != 8*512) {
			return -2;
		}
		for(uint64_t l2 = 0; l2 < 512; l2++) {
			if(!(l2buf[l2] & PG_PRESENT_MASK)) {
				continue;
			}
			if(l2buf[l2] & PG_PSE_MASK) {
				continue;
			}
			if(read_sim_memory(amount_mems, mems, (l2buf[l2] & 0x3fffffffff000ULL), 8*512, l3buf) != 8*512) {
				return -3;
			}
			for(uint64_t l3 = 0; l3 < 512; l3++) {
				if(l3 == 0) {
					printf("L3: 0x%012lx\n", l3buf[l3]);
				}
				if(!(l3buf[l3] & PG_PRESENT_MASK)) {
					continue;
				}
				if(read_sim_memory(amount_mems, mems, (l3buf[l3] & 0x3fffffffff000ULL), 8*512, l4buf) != 8*512) {
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
	return 0;
}

int read_pagetable(int amount_mems, struct Memory* mems, char* path) {
	FILE* f = fopen(path, "r");
	uint64_t totalLevelsRead = 0;
	if(!f) {
		return 1;
	}
	uint64_t physaddr;
	uint64_t buf[1<<12];
	bool first = true;
	while(true){
		if(fread(&physaddr, sizeof(uint64_t), 1, f) != 1) {
			printf("Could not read physaddr\n");
			//TODO verify EOF
			break;
		}
		if(first){
			printf("Read CR3: 0x%012lx\n", physaddr);
		}
		size_t size = 512;
		if(fread(buf, sizeof(uint64_t), size, f) != size) {
			printf("Could not read pagetable\n");
			return 1;
		}
#ifdef DEBUG
/*			for(int i = 0; i < 512; i++) {
				printf("Read[%x]=0x%012lx storing at: 0x%012lx\n", i, buf[i], (8*i)+(physaddr&0x3fffffffff000ULL));
			}*/
#endif
		write_sim_memory(amount_mems, mems, physaddr & 0x3fffffffff000ULL, size*8, buf);
		totalLevelsRead++;
		first = false;
	}
	printf("Total subtables:%ld\n", totalLevelsRead);
	return 0;

}
