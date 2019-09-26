#include <stdio.h>
#include <string.h>

#include "memory.h"

struct memory* init_memory() {
        struct memory* mem = malloc(sizeof(struct memory));
        mem->table = calloc((1<<12), sizeof(uint64_t));
        return mem;
}

void free_memory(struct memory* mem) {
	//TODO free with dfs
}

int read_in_level(struct memory* mem, uint64_t address, size_t size, uint8_t* value, uint8_t level) {
	if(mem == NULL){
		return 0;
	}
	uint64_t level_addr = (address >> (level*12)) & 0xFFF;
	if( (level_addr + size) > (1<<12)){
		printf("BORDER CROSSED!\n");
		size_t bytes_first_level = (1<<12) - level_addr;
		size_t bytes_second_level = size - bytes_first_level;
		int read_first_level = read_in_level((struct memory*)mem->table[level_addr], address, bytes_first_level, value, level-1);
		int read_second_level = read_in_level((struct memory*)mem->table[level_addr+1], address, bytes_second_level,value+bytes_first_level, level-1);
		return read_first_level + read_second_level;
	}
	if(level == 0) {
		if(size + level_addr > (1 << 12)) {
			//TODO handle memory writes accross boundaries
			printf("Something is going wrong, trying to read outside of memory boundary!\n");
			return 0;
		}
		memcpy(value, mem->table+level_addr, size);
		return size;
	}
	//Lookup next level
	return read_in_level((struct memory*)mem->table[level_addr], address, size, value, level-1);
}

int read(struct memory* mem, uint64_t address,  size_t size, uint8_t* value){
	return read_in_level(mem, address, size, value, 3);
}

int write_in_level(struct memory* mem, uint64_t address, size_t size, uint8_t* value, uint8_t level) {
	if(level == 3 && mem == NULL) {
		return 0;
	}
	uint64_t level_addr = (address >> (level*12)) & 0xFFF;
	if(level == 0 ){
		if(size + level_addr > (1<<12)) {
			//TODO handle memory writes accross boundaries
			printf("Something goes wrong, trying to write accross boundareis\n");
			return 0;
		}
		memcpy(mem->table+level_addr, value, size);
		return size;
	}
	if((struct memory*)mem->table[level_addr] == NULL ){
		mem->table[level_addr] = (uint64_t)init_memory();
	}
	return write_in_level((struct memory*)mem->table[level_addr], address, size, value, level-1);

}

int write(struct memory* mem, uint64_t address, size_t size, uint8_t* value){
	return write_in_level(mem, address, size, value, 3);
}


