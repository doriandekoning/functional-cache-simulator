#!/bin/bash


#for i in {1..3}
#do
    time mpirun -v -np 8 bin/distributed_simulator /home/dorian/go/src/github.com/doriandekoning/run-qemu-cache-distributed-sim/trace_mapping_use $1/crvalues $1/trace $1/memrange 4 $1/cachesimout $1/memdump
#done
