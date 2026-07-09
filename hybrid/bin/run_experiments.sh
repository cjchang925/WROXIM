#!/bin/bash

set -e

# buffer_depth -> simulation_time
#   8 -> 45683
#   16 -> 46066
#   32 -> 40022

ARCH="GWOR"
network_protocol="ABP"
buffer_depth="8"
sim_time="10000"

# for i in  "8" "16" "32"; do     #   buffer_depth
#     ./noxim -config ../tm_examples/01-16-nownoc.yaml -architecture ${ARCH} -network_protocol ${network_protocol} -buffer ${i} -taskmapping ../tm_examples/tg11_0_0_0m.map -taskmappinglog ../tm_log/tg11_${ARCH}_b${i}_r_pt.log > ../tm_log/tmp_output.log
# done


#   "LAMBDA" "GWOR" "LIGHT" "SNAKE"
# for ARCH in "MESH" "LAMBDA" "GWOR" "LIGHT" "SNAKE"; do
#     for buffer_depth in "8" "16" "32";do
#         for tg_idx in $(seq 0 5);do

#             ./noxim -config ../tm_examples/01-16-nownoc.yaml -architecture ${ARCH} -network_protocol ${network_protocol} -buffer ${buffer_depth} -sim ${sim_time} -taskmapping ../tm_examples/experiments/experiment_tg${tg_idx}.map -taskmappinglog ../tm_log/experiments/tg${tg_idx}/experiment_tg${tg_idx}_${ARCH}_b${buffer_depth}.log > ../tm_log/tmp_output.log
#         done
#     done
# done


tg_idx="5"

./noxim -config ../tm_examples/01-16-nownoc.yaml -architecture ${ARCH} -network_protocol ${network_protocol} -buffer ${buffer_depth} -sim ${sim_time} -taskmapping ../tm_examples/experiments/experiment_tg${tg_idx}.map -taskmappinglog ../tm_log/experiments/tg${tg_idx}/experiment_tg${tg_idx}_${ARCH}_b${buffer_depth}.log > ../tm_log/tmp_output.log
        
