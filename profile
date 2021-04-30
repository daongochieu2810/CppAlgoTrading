#!/bin/bash

#pprof --gv $1 /tmp/heapprof.0001.heap #this line is for heap profiling
pprof --gv $1 /tmp/prof.out
