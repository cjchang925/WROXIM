#!/bin/bash

set -e

# buffer_depth -> simulation_time
#   8 -> 45683
#   16 -> 46066
#   32 -> 40022

# ARCH="MESH"
# network_protocol="ABP"
buffer_depth="16"
# sim_time="100000"

# for i in  "8" "16" "32"; do     #   buffer_depth
#     ./noxim -config ../tm_examples/01-16-nownoc.yaml -architecture ${ARCH} -network_protocol ${network_protocol} -buffer ${i} -taskmapping ../tm_examples/tg11_0_0_0m.map -taskmappinglog ../tm_log/tg11_${ARCH}_b${i}_r_pt.log > ../tm_log/tmp_output.log
# done


#   "LAMBDA" "GWOR" "LIGHT" "SNAKE"
for ARCH in "LAMBDA" "GWOR"; do
    for tg_idx in "aes" "fft" "mmul";do
        for buffer_depth in "8" "16" "32" "64";do
            case "${tg_idx}" in
                "aes")
                    sim_time=10000
                    ;;
                "fft")
                    sim_time=30000
                    ;;
                "mmul")
                    sim_time=140000
                    ;;  
                *)
                    echo "Unknown mapfile: ${mapfile}"
                    continue
                    ;;
            esac

            echo "ARCH = MESH, buffer_depth = ${buffer_depth}, tg_idx = ${tg_idx}"
            ./noxim -config ../tm_examples/01-16-nownoc.yaml -architecture MESH -buffer ${buffer_depth} -sim ${sim_time} -taskmapping ../tm_examples/experiments2/tg_${tg_idx}.map -taskmappinglog ../tm_log/tgff/tg_${tg_idx}/MESH/tg_${tg_idx}_MESH_b${buffer_depth}.log > ../tm_log/tmp_output.log
        
            for network_protocol in "ABP" "ON_OFF_FLOWCONTROL"; do
                echo "ARCH = ${ARCH}, buffer_depth = ${buffer_depth}, tg_idx = ${tg_idx}, network_protocol=${network_protocol}"
                ./noxim -config ../tm_examples/01-16-nownoc.yaml -architecture ${ARCH} -network_protocol ${network_protocol} -buffer ${buffer_depth} -sim ${sim_time} -taskmapping ../tm_examples/experiments2/tg_${tg_idx}.map -taskmappinglog ../tm_log/tgff/tg_${tg_idx}/${ARCH}/tg_${tg_idx}_${ARCH}_${network_protocol}_b${buffer_depth}.log > ../tm_log/tmp_output.log
            done
        done
    done
done
