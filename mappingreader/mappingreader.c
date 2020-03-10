#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "mappingreader.h"

int read_mapping(char* input_file_location, struct EventIDMapping* mapping) {
  FILE * mapping_fp;
  char event_name[256];
  printf("Reading trace mapping at: \"%s\"\n", input_file_location);
  mapping_fp = fopen(input_file_location, "r");
  if(mapping_fp == NULL) {
    return 1;
  }
  uint8_t event_id;
  uint32_t len;
  while(true) {
    if(fread(&event_id, 1, 1, mapping_fp) != 1 || fread(&len, 4, 1, mapping_fp) != 1) {
      return 0;
    }
    event_id &= (0x3F);


    if(len >= 256) {
      printf("Buffer size too small!\n");
      return 2;
    }
    if(fread(&event_name, len, 1, mapping_fp) != 1) {
      return 3;
    }

    event_name[len] = '\0';
    if(!strcmp(event_name, "guest_mem_load_before_exec")) {
      mapping->guest_mem_load_before_exec = event_id;
    } else if(!strcmp(event_name, "guest_mem_store_before_exec")) {
      mapping->guest_mem_store_before_exec = event_id;
    } else if(!strcmp(event_name, "guest_update_cr")) {
      mapping->guest_update_cr = event_id;
    } else if(!strcmp(event_name, "guest_flush_tlb_invlpg")){
      mapping->guest_flush_tlb_invlpg = event_id;
    } else if(!strcmp(event_name, "guest_start_exec_tb")) {
      mapping->guest_start_exec_tb = event_id;
    }
  }
  fclose(mapping_fp);
  return 0;
}

void print_mapping(struct EventIDMapping* mapping) {
  printf("Trace event ID Mapping:\n");
  printf("%d:\tguest_mem_load_before_exec\n", mapping->guest_mem_load_before_exec);
  printf("%d:\tguest_mem_store_before_exec\n", mapping->guest_mem_store_before_exec);
  printf("%d:\tguest_update_cr\n", mapping->guest_update_cr);
  printf("%d:\tguest_flush_tlb_invlpg\n", mapping->guest_flush_tlb_invlpg);
  printf("%d:\tuest_start_exec_tb\n", mapping->guest_start_exec_tb);
}
