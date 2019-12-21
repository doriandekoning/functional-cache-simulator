#include "mappingreader.h"
#include <stdio.h>

int main() {
  char* mapping_location = "/home/dorian/go/src/github.com/doriandekoning/run-qemu-cache-distributed-sim/trace_mapping";
  struct EventIDMapping mapping;
  int ret = read_mapping(mapping_location, &mapping);
  if(ret != 0) {
    printf("Unable to read mapping!\n");
    return 1;
  }
  printf("Read mapping!\n");
  printf("cr_change: %d\n", mapping.guest_update_cr);
  printf("instruction_fetch: %d\n", mapping.instruction_fetch);
  printf("guest_mem_store_before: %d\n", mapping.guest_mem_before_exec);
  printf("guest_mem_load_before: %d\n", mapping.guest_mem_load_before_exec);
  printf("guest_mem_before_exec: %d\n", mapping.guest_mem_store_before_exec);
  return 0;

}
