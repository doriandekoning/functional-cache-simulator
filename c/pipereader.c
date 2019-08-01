#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cachestate.h"
#include "pipereader.h"
#include <fcntl.h>
#include <unistd.h>





#define READ_UINT64_FROM_PIPE(variable) if(fread(&variable, 1, 8, pipe) != 8) {printf("Could not read from pipe!\n");return NULL;}
#define READ_UINT32_FROM_PIPE(variable) if(fread(&variable, 1, 4, pipe) != 4) {printf("Could not read from pipe!\n");return NULL;}
#define READ_STRING_FROM_PIPE(variable, length) if(fread(variable, 1, length, pipe) != length) {printf("Could not read from pipe!\n");return NULL;}

void* read_header(FILE * pipe) {
  // Read eventid
    uint64_t eventid;
    READ_UINT64_FROM_PIPE(eventid)
    if(eventid != 0xffffffffffffffff) {
        printf("Magicnumber is wrong %llx\n", eventid);
        return NULL;
    }

    // Read trace header
    uint64_t magicnumber;
    READ_UINT64_FROM_PIPE(magicnumber)
    if(magicnumber != 0xf2b177cb0aa429b4) {
        printf("Magicnumber is wrong %llx\n", magicnumber);
        return NULL;
    }

    uint64_t headerversion;
    READ_UINT64_FROM_PIPE(headerversion)
    if (headerversion != 4) {
        printf("Wrong header version!\n");
        return NULL;
    }
    return (void*) -1;
}


cache_access* get_next_access(FILE* pipe, cache_access* access) {//TODO return type int
    do{
        uint64_t record_type;
        READ_UINT64_FROM_PIPE(record_type)
        if( record_type == 0) {
            uint64_t event_id;
            READ_UINT64_FROM_PIPE(event_id)
            uint32_t length;
            READ_UINT32_FROM_PIPE(length)
            char* text = malloc(length);
            READ_STRING_FROM_PIPE(text, length)
        } else if (record_type == 1) {
            uint64_t event_id;
            READ_UINT64_FROM_PIPE(event_id)
            if(event_id != 75 && event_id != (uint64_t)0xfffffffffffffffe) {
                printf("Only traces with eventID 75 are supported, found event with id: %llu\n", event_id);
                return NULL;
            }
            READ_UINT64_FROM_PIPE(access->tick);
            uint32_t rec_len; //Unused
            READ_UINT32_FROM_PIPE(rec_len)
            uint32_t trace_pid; //Unused
            READ_UINT32_FROM_PIPE(trace_pid)

            READ_UINT64_FROM_PIPE(access->cpu)
            READ_UINT64_FROM_PIPE(access->address)
            struct qemu_mem_info info;
            READ_UINT64_FROM_PIPE(info)
            uint64_t size =  (1 << (info.size_shift));
            if(size > 64) {printf("Size larger than cachelinesize!");}
            access->write =  info.store;


            return 0;
        }   else {
            printf("Unknown record type encountered!\n");
            return NULL;
        }
    }while(true);
}