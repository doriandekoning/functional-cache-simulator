#include <stdio.h>
#include <stdint.h>


#define ADDRESS_TAG(addr) ( (addr >> 6) % (1 << 13)) 
#define ADDRESS_OFFSET(addr)  addr % (1 << 7)
#define ADDRESS_INDEX(addr) (addr >> (6+12))

uint64_t setupmask(unsigned from, unsigned to) {
	uint64_t mask = 0;
	for(unsigned i = from; i <=to; i++) {
		if(from==18){printf("%d, %llx\n",i, mask);}
		mask |= (1ULL << i);
	}
	return mask;
}

uint64_t ADDRESS_OFFSET_MASK = 0;
uint64_t ADDRESS_TAG_MASK = 0;
uint64_t ADDRESS_INDEX_MASK = 0;

void printaddr(uint64_t addr) {
	uint64_t tag = ADDRESS_TAG(addr);
	uint64_t offset = ADDRESS_OFFSET(addr);
	uint64_t index = ADDRESS_INDEX(addr);
	printf("Tag:%llu\nOffset:%llu\nIndex:%llu\n", tag, offset, index);
}

int main() {
	init_cachestate_masks(12, 6);
	uint64_t test1 = 23  | (321 << 13) | (1234 << 17);
	printaddr(test1);
	uint64_t test2 = (321 << 12);
	printaddr(test2);
	printf("%llx\n", ADDRESS_OFFSET_MASK);
	printf("%llx\n", ADDRESS_TAG_MASK);
	printf("%llx\n", ADDRESS_OFFSET_MASK + ADDRESS_TAG_MASK);
	printf("%llx\n", ADDRESS_INDEX_MASK);
	printf("%llx\n", (uint64_t)18446744073709551615);
	printf("A%llx\n", ADDRESS_OFFSET_MASK | ADDRESS_TAG_MASK | ADDRESS_INDEX_MASK);
	return 0;
}


