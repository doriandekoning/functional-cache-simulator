#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <byteswap.h>
#include "cache/state.h"
#include "pipereader.h"


#define READ_UINT8_FROM_PIPE(variable)  if(fread(&variable, 1, 1, pipe) != 1){printf("Could not read uint8 from pipe!\n");return -1;}
#define READ_UINT16_FROM_PIPE(variable) if(fread(&variable, 2, 1, pipe) != 1) {printf("Could not read uint16 from pipe!\n");return -1;}
#define READ_UINT32_FROM_PIPE(variable) if(fread(&variable, 4, 1, pipe) != 1) {printf("Could not read uint32 from pipe!\n");return -1;}
#define READ_UINT48_FROM_PIPE(variable) if(fread(&variable, 6, 1, pipe) != 1) {printf("Could not read uint48 from pipe!\n"); return -1;}
#define READ_UINT64_FROM_PIPE(variable) if(fread(&variable, 8, 1, pipe) != 1) {printf("Could not read uint64 from pipe!\n");return -1;}
#define READ_BYTES_FROM_PIPE(variable, length) if(fread(variable, 1, length, pipe) != length) {printf("Could not read from pipe!\n");return -1;}

#define UINT_FROM_BUF(type, index) *(type*)(buf+index)

#define INFO_SIZE_SHIFT_MASK 0xf /* size shift mask */
#define INFO_SIGN_EXTEND_MASK (1ULL << 4)    /* sign extended (y/n) */
#define INFO_BIG_ENDIAN_MASK (1ULL << 5)    /* big endian (y/n) */
#define INFO_STORE_MASK (1ULL << 6)    /* store (y/n) */

uint8_t buf[128];

void get_mem_info(uint16_t raw_info, struct qemu_mem_info* info_struct) {
	info_struct->size_shift = raw_info & INFO_SIZE_SHIFT_MASK;
	info_struct->store = raw_info & INFO_STORE_MASK;
	info_struct->endianness = raw_info & INFO_BIG_ENDIAN_MASK;
	info_struct->sign_extend = raw_info & INFO_SIGN_EXTEND_MASK;
}

int read_header(FILE* pipe) {
  // Read eventid
    uint64_t eventid;
    READ_UINT64_FROM_PIPE(eventid)
    if(eventid != 0xffffffffffffffff) {
        printf("Eventid is wrong %llx\n", eventid);
        return 1;
    }

    // Read trace header
    uint64_t magicnumber;
    READ_UINT64_FROM_PIPE(magicnumber)
    if(magicnumber != 0xf2b177cb0aa429b4) {
        printf("Magicnumber is wrong %llx\n", magicnumber);
        return 2;
    }

    uint64_t headerversion;
    READ_UINT64_FROM_PIPE(headerversion)
    if (headerversion != 4) {
        printf("Wrong header version!\n");
        return 3;
    }
    return 0;
}

uint64_t get_memory_access(FILE* pipe, cache_access* access, bool write) {
  READ_BYTES_FROM_PIPE(buf, 1 + 8 + 2); // cpu(1) + address(8) + info_int(2)
  access->cpu = UINT_FROM_BUF(uint8_t, 0);
  access->address = UINT_FROM_BUF(uint64_t, 1);
	get_mem_info(UINT_FROM_BUF(uint16_t, 9), (struct qemu_mem_info*)buf+32);
	access->type = (UINT_FROM_BUF(struct qemu_mem_info, 32)).store ? CACHE_WRITE : CACHE_READ;
	access->size = (1 << (UINT_FROM_BUF(struct qemu_mem_info, 32)).size_shift);
  access->big_endian = UINT_FROM_BUF(uint8_t, 32) & INFO_BIG_ENDIAN_MASK;
  access->user_access = (UINT_FROM_BUF(uint8_t, 10) == MMU_USER_IDX);

	if(write) {
    READ_UINT8_FROM_PIPE(access->location);
		READ_UINT64_FROM_PIPE(access->data);
	}
	return 0;
}

uint64_t get_cr_change(FILE* pipe, cr_change* change){
  READ_BYTES_FROM_PIPE(buf, 1+1+8) // Read cpu(1) + register_number(1) + new_value(8)
	change->cpu = UINT_FROM_BUF(uint8_t, 0);
	change->register_number = UINT_FROM_BUF(uint8_t, 1);
	change->new_value = UINT_FROM_BUF(uint64_t, 2);
	return 0;
}

int get_next_event_id(FILE* pipe, uint64_t* delta_t, bool* negative_delta_t, uint8_t* event_id) {
  uint16_t delta_t_lsb;
  READ_UINT16_FROM_PIPE(delta_t_lsb);
  if(delta_t_lsb & (1 << 7)) {
    READ_UINT48_FROM_PIPE(*delta_t);
    *delta_t = (delta_t_lsb & 0x7fff) | (*delta_t << 16);
    *delta_t = __bswap_64(*delta_t) &  ~(0x1ULL << 63 );

  }else{
    *delta_t = 0x0ULL;
    *delta_t |= (__bswap_16(delta_t_lsb) & 0x7fff);
  }
  READ_UINT8_FROM_PIPE(*event_id);

  *negative_delta_t = (*event_id & (1 << 7)) >> 7;
  *event_id &= 0x7f; //filter deltat+- bit
  return 0;
}



