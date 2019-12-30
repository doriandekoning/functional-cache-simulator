#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "mappingreader.h"

int read_mapping(char* input_file_location, struct EventIDMapping* mapping) {
  FILE * mapping_fp;
  char event_name[256];

  mapping_fp = fopen(input_file_location, "r");
  if(mapping_fp == NULL) {
    return 1;
  }
  uint8_t event_id;
  uint32_t len;
  while(true) {
    if(fread(&event_id, 1, 1, mapping_fp) != 1 || fread(&len, 1, 4, mapping_fp) != 4) {
      return 0;
    }
    event_id &= (0x3F);
    if(len >= 256) {
      printf("Buffer size too small!\n");
      return 1;
    }
    if(fread(&event_name, 1, len, mapping_fp) != len) {
      printf("Unable to read name!\n");
      return 1;
    }
    event_name[len] = '\0';
    if(!strcmp(event_name, "guest_mem_load_before_exec")) {
      mapping->guest_mem_load_before_exec = event_id;
    } else if(!strcmp(event_name, "guest_mem_store_before_exec")) {
      mapping->guest_mem_store_before_exec = event_id;
    } else if(!strcmp(event_name, "guest_update_cr")) {
      mapping->guest_update_cr = event_id;
    } //TODO instruction fetch
  }
  fclose(mapping_fp);
  return 0;
}



