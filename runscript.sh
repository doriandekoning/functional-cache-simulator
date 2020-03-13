#!/bin/bash


prun -np $1 -script $PRUN_ETC/prun-openmpi bin/distributed_simulator /var/scratch/tdkoning/trace/mapping /var/scratch/tdkoning/trace/$2/crvalues /var/scratch/tdkoning/trace/$2/trace /var/scratch/tdkoning/trace/$2/memrange $3 /var/scratch/tdkoning/trace/$2/cachesimout /var/scratch/tdkoning/trace/$2/memdump

