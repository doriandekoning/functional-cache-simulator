#include <stdio.h>
#include <stdint.h>

int main() {
	FILE* f;
	f = fopen("custompagetable", "w");
	if(f == NULL){
		printf("Could not open file!");
	}

	uint64_t phys;
	uint64_t buf[512];
	for(int i = 0; i < 512; i++) {
		buf[i] = 0;
	}
	for(int i = 1; i < 5; i++) {
		phys = i* 0xFFFF000 | 0x67 ;
		if(fwrite(&phys, sizeof(uint64_t), 1, f) != 1) {
			printf("Couldnt write phys!\n");
			return 1;
		}
		buf[0xab] = phys + 0xFFFF000;
		if(fwrite(buf, sizeof(uint64_t), 512, f) != 512) {
			printf("Couldnt write buf!\n");
			return 1;
		}
	}
	fflush(f);
}
