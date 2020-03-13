#!/bin/bash

mpicc -lrt -g -fmax-errors=5 \
-DAMOUNT_SIMULATED_PROCESSORS=4 -DINPUT_FILE=1 -DSIMULATE_CACHE=1 -DADDRESS_TRANSLATION -DUSING_MPI -DMULTI_LEVEL_MEMORY \
-I. -o bin/distributed_simulator\
 simulator/distributed/*.c pagetable/pagetable.c memory/*.c mappingreader/*.c filereader/*.c  control_registers/*.c cache/*.c
