#!/bin/bash


for i in {0..2}
do
    time bin/simple-functional-cache-sim /home/dorian/go/src/github.com/doriandekoning/run-qemu-cache-distributed-sim/trace_mapping_use  $1/trace $1/output  $1/memdump $1/memrange $1/crvalues
done
