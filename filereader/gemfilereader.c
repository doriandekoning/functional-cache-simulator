#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <byteswap.h>
#include "cache/state.h"
#include "filereader.h"
#include "traceevents.h"

#define READ_UINT8_FROM_FILE(variable)  if(fread(&variable, 1, 1, file) != 1){printf("Could not read uint8 from file:%lu!\n", ftell(file));return -1;}
#define READ_UINT16_FROM_FILE(variable) if(fread(&variable, 2, 1, file) != 1) {printf("Could not read uint16 from file:%lu!\n",ftell(file));return -1;}
#define READ_UINT32_FROM_FILE(variable) if(fread(&variable, 4, 1, file) != 1) {printf("Could not read uint32 from file:%lu!\n",ftell(file));return -1;}
#define READ_UINT48_FROM_FILE(variable) if(fread(&variable, 6, 1, file) != 1) {printf("Could not read uint48 from file:%lu!\n",ftell(file)); return -1;}
#define READ_UINT64_FROM_FILE(variable) if(fread(&variable, 8, 1, file) != 1) {printf("Could not read uint64 from file:%lu!\n",ftell(file));return -1;}
#define READ_BYTES_FROM_FILE(variable, length) if(fread(variable, 1, length, file) != length) {printf("Could not read from file:%lu!\n",ftell(file));return -1;}

#define UINT_FROM_BUF(type, index) *(type*)(buf+index)

uint8_t buf[128];

uint64_t file_get_gem_memory_access(FILE* file, cache_access* access) {
  READ_BYTES_FROM_FILE(buf, 1 + 8 + 8 + 1); // type(1) + tick(8) + address(8) + cpu(1)
  access->tick = UINT_FROM_BUF(uint64_t, 0);
  access->address = UINT_FROM_BUF(uint64_t, 8);
  uint8_t type = UINT_FROM_BUF(uint8_t, 16);
  if(type == 2) {
    access->type = CACHE_EVENT_INSTRUCTION_FETCH;
  }else if(type == 0) {
    access->type = CACHE_EVENT_READ;
  }else{
    access->type = CACHE_EVENT_WRITE;
  }
  access->cpu = UINT_FROM_BUF(uint8_t, 17);
	return 0;
}

