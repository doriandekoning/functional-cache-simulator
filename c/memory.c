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

int read_in_level(struct memory* top_level, struct memory* mem, uint64_t address, size_t size, uint8_t* value, uint8_t level) {
	if(mem == NULL){
		return 0;
	}
	uint64_t level_addr = ((address & 0x8FFFFFFFFFFF) >> (level*12)) & 0xFFF;
	if( (level_addr + size) > (1<<12)){
		size_t bytes_level_left = (1<<12) - level_addr;
		size_t bytes_level_right = size - bytes_level_left;
                struct memory* m = (struct memory*)mem->table[level_addr];
                if(mem->table[level_addr] == 0) {
			printf("Trying to read from non initialized memory!\n");
			return 0;
		}
                printf("%piets\n", (struct memory*)mem->table[level_addr]);
                int left_bytes_read = read_in_level(top_level, (struct memory*)mem->table[level_addr], address, bytes_level_left, value, level-1);
                int right_bytes_read =  read_in_level(top_level, top_level, address+bytes_level_left, bytes_level_right, &(value[bytes_level_left]), 3);
                return left_bytes_read + right_bytes_read;
	}
	if(level == 0) {
		memcpy(value, mem->table+level_addr, size);
		return size;
	}
	//Lookup next level
	return read_in_level(top_level, (struct memory*)mem->table[level_addr], address, size, value, level-1);
}

int read_sim_memory(struct memory* mem, uint64_t address, size_t size, uint8_t* value){
	return read_in_level(mem, mem, address, size, value, 3);
}

int write_in_level(struct memory* top_level, struct memory* mem, uint64_t address, size_t size, uint8_t* value, uint8_t level) {
	if(level == 3 && mem == NULL) {
		return 0;
	}
	uint64_t level_addr = ((address & 0x8FFFFFFFFFFF) >> (level*12)) & 0xFFF;
	if((level_addr + size ) > (1 << 12)) {
		size_t bytes_left_level = (1 << 12) - level_addr;
		size_t bytes_right_level = size - bytes_left_level;
		struct memory* m = (struct memory*)mem->table[level_addr];
		if(mem->table[level_addr] == 0) {
			mem->table[level_addr] = (uint64_t)init_memory();
		}
		int write_left_level = write_in_level(top_level, (struct memory*)mem->table[level_addr], address, bytes_left_level, value, level-1);
		int write_right_level = write_in_level(top_level, top_level, address+bytes_left_level, bytes_right_level, &(value[bytes_left_level]), 3);
		return write_left_level + write_right_level;
	}
	if(level == 0 ){
		memcpy(mem->table+level_addr, value, size);
		return size;
	}
	if((struct memory*)mem->table[level_addr] == NULL ){
		mem->table[level_addr] = (uint64_t)init_memory();
	}
	return write_in_level(top_level, (struct memory*)mem->table[level_addr], address, size, value, level-1);

}

int write_sim_memory(struct memory* mem, uint64_t address, size_t size, uint8_t* value){
	return write_in_level(mem, mem, address, size, value, 3);
}

