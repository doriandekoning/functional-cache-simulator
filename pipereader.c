#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cachestate.h"
#include "pipereader.h"
#include <unistd.h>
#include <stdint.h>



#define READ_UINT8_FROM_PIPE(variable) if(fread(&variable, 1, 1, pipe) != 1){return -1;}
#define READ_UINT32_FROM_PIPE(variable) if(fread(&variable, 4, 1, pipe) != 1) {printf("Could not read uint32 from pipe!\n");return -1;}
#define READ_UINT64_FROM_PIPE(variable) if(fread(&variable, 8, 1, pipe) != 1) {printf("Could not read uint64 from pipe!\n");return -1;}
#define READ_STRING_FROM_PIPE(variable, length) if(fread(variable, 1, length, pipe) != length) {printf("Could not read from pipe!\n");return -1;}

#define INFO_SIZE_SHIFT_MASK 0x7 /* size shift mask */
#define INFO_SIGN_EXTEND_MASK (1ULL << 3)    /* sign extended (y/n) */
#define INFO_BIG_ENDIAN_MASK (1ULL << 4)    /* big endian (y/n) */
#define INFO_STORE_MASK (1ULL << 5)    /* store (y/n) */

struct qemu_mem_info* get_mem_info(uint8_t raw_info) {
	struct qemu_mem_info* info = malloc(sizeof(struct qemu_mem_info));
	info->size_shift = raw_info & INFO_SIZE_SHIFT_MASK;
	info->store = raw_info & INFO_STORE_MASK;
	info->endianness = raw_info & INFO_BIG_ENDIAN_MASK;
	info->sign_extend = raw_info & INFO_SIGN_EXTEND_MASK;
	return info;
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

int get_cache_access(FILE* pipe, cache_access* access) {
	READ_UINT64_FROM_PIPE(access->tick);
	READ_UINT64_FROM_PIPE(access->cpu)
	READ_UINT64_FROM_PIPE(access->address);
	uint8_t info_int;
	struct qemu_mem_info* info; //TODO do not allocate a new one every time
	READ_UINT8_FROM_PIPE(info_int);
	info = get_mem_info(info_int);
	access->type = info->store ? CACHE_WRITE : CACHE_READ;
	access->size = (1 << info->size_shift);
	if(info->store) {
		READ_UINT64_FROM_PIPE(access->data);
	}
	free(info);
	return 0;
}

int get_cr_change(FILE* pipe, cr_change* change){
	READ_UINT64_FROM_PIPE(change->tick);
	READ_UINT64_FROM_PIPE(change->cpu);
	READ_UINT8_FROM_PIPE(change->register_number);
	READ_UINT64_FROM_PIPE(change->new_value);
	return 0;
}

uint8_t get_next_event_id(FILE* pipe) {
  do{
    uint8_t record_type;
    READ_UINT8_FROM_PIPE(record_type)
  	uint8_t event_id;
	  READ_UINT8_FROM_PIPE(event_id)
    if( record_type == 0) {
		  uint32_t length;
		  READ_UINT32_FROM_PIPE(length)
		  char* text = malloc(length);
		   READ_STRING_FROM_PIPE(text, length)
     } else if (record_type == 1) {
		   return event_id;
     } else {
       debug_printf("Unknown record type: %lx encountered!\n", record_type);
      return -1;
     }
    }while(true);
}
