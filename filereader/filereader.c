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

int file_read_header(FILE* file) {
  // Read eventid
    uint64_t eventid;
    READ_UINT64_FROM_FILE(eventid)
    if(eventid != 0xffffffffffffffff) {
        printf("Eventid is wrong %lx\n", eventid);
        return 1;
    }

    // Read trace header
    uint64_t magicnumber;
    READ_UINT64_FROM_FILE(magicnumber)
    if(magicnumber != 0xf2b177cb0aa429b4) {
        printf("Magicnumber is wrong %lx\n", magicnumber);
        return 2;
    }

    uint64_t headerversion;
    READ_UINT64_FROM_FILE(headerversion)
    if (headerversion != 4) {
        printf("Wrong header version!\n");
        return 3;
    }
    return 0;
}

uint64_t file_get_memory_access(FILE* file, cache_access* access, bool write) {
  READ_BYTES_FROM_FILE(buf, 1 + 8 + 1); // cpu(1) + address(8) + info_int(1)
  access->cpu = UINT_FROM_BUF(uint8_t, 0);
  access->address = UINT_FROM_BUF(uint64_t, 1);
  // access->physaddress = UINT_FROM_BUF(uint64_t, 9);
  uint8_t info =  UINT_FROM_BUF(uint8_t, 9);
	access->type = (info & INFO_STORE_MASK) ? CACHE_EVENT_WRITE : CACHE_EVENT_READ;
	access->size = (1 << (info & INFO_SIZE_SHIFT_MASK));
	if(write) {
		READ_UINT64_FROM_FILE(access->data);
	}
	return 0;
}

uint64_t file_get_tb_start_exec(FILE* file, tb_start_exec* tb_start_exec) {
  READ_BYTES_FROM_FILE(buf, 1 + 8 + 2);
  tb_start_exec->cpu = UINT_FROM_BUF(uint8_t, 0);
  tb_start_exec->pc = UINT_FROM_BUF(uint64_t, 1);
  tb_start_exec->size = UINT_FROM_BUF(uint16_t, 9);
  return 0;
}

uint64_t file_get_cr_change(FILE* file, cr_change* change){
  READ_BYTES_FROM_FILE(buf, 1+1+8) // Read cpu(1) + register_number(1) + new_value(8)
	change->cpu = UINT_FROM_BUF(uint8_t, 0);
	change->register_number = UINT_FROM_BUF(uint8_t, 1);
	change->new_value = UINT_FROM_BUF(uint64_t, 2);
	return 0;
}

uint64_t file_get_invlpg(FILE* file, uint64_t* addr, uint8_t* cpu) {
  READ_BYTES_FROM_FILE(buf, 8 +1);
  *addr = UINT_FROM_BUF(uint64_t, 0);
  *cpu = UINT_FROM_BUF(uint8_t, 8);
  return 0;
}

int file_get_next_event_id(FILE* file, uint64_t* delta_t, bool* negative_delta_t, uint8_t* event_id) {
  uint16_t delta_t_lsb;
  READ_UINT16_FROM_FILE(delta_t_lsb);
  if(delta_t_lsb & (1 << 7)) {
    READ_UINT48_FROM_FILE(*delta_t);
    *delta_t = (delta_t_lsb & 0x7fff) | (*delta_t << 16);
    *delta_t = __bswap_64(*delta_t) &  ~(0x1ULL << 63 );

  }else{
    *delta_t = 0x0ULL;
    *delta_t |= (__bswap_16(delta_t_lsb) & 0x7fff);
  }
  READ_UINT8_FROM_FILE(*event_id);
  *negative_delta_t = (*event_id & (1 << 7)) >> 7;
  *event_id &= 0x7f; //filter deltat+- bit
  return 0;
}



