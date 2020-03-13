#!/bin/bash


#for i in {1..3}
#do
    echo bin/simple-functional-cache-sim /home/dorian/go/src/github.com/doriandekoning/run-qemu-cache-distributed-sim/trace_mapping_use  $2/trace $2/output $1 $2/memdump $2/memrange $2/crvalues
    time bin/simple-functional-cache-sim /home/dorian/go/src/github.com/doriandekoning/run-qemu-cache-distributed-sim/trace_mapping_use  $2/trace $2/output $1 $2/memdump $2/memrange $2/crvalues
#done
