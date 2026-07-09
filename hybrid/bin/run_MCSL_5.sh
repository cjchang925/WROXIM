#!/bin/bash

set -e

# buffer_depth -> simulation_time
#   8 -> 45683
#   16 -> 46066
#   32 -> 40022

# ARCH="LAMBDA"
# network_protocol="ABP"
buffer_depth="64"
# sim_time="100000"

# for i in  "8" "16" "32"; do     #   buffer_depth
#     ./noxim -config ../tm_examples/01-16-nownoc.yaml -architecture ${ARCH} -network_protocol ${network_protocol} -buffer ${i} -taskmapping ../tm_examples/tg11_0_0_0m.map -taskmappinglog ../tm_log/tg11_${ARCH}_b${i}_r_pt.log > ../tm_log/tmp_output.log
# done

# for ARCH in "MESH" "LAMBDA"; do

# "FFT-1024_complex_mesh_4x4_stp" "Sparse_mesh_4x4_stp"
# "Fpppp_mesh_4x4_stp" "H264-720p_dec_mesh_4x4_stp" "H264-1080p_dec_mesh_4x4_stp" "Robot_mesh_4x4_stp" "RS-32_28_8_dec_mesh_4x4_stp" "RS-32_28_8_enc_mesh_4x4_stp" 
for mapfile in "FFT-1024_complex_mesh_4x4_stp" "Sparse_mesh_4x4_stp" "RS-32_28_8_dec_mesh_4x4_stp"; do

    # 根據 mapfile 名稱設定 sim_time
    case "${mapfile}" in
        "FFT-1024_complex_mesh_4x4_stp")
            sim_time=230000
            ;;
        "Sparse_mesh_4x4_stp")
            sim_time=150000
            ;;
        "Robot_mesh_4x4_stp")
            sim_time=490000
            ;;  
        "Fpppp_mesh_4x4_stp")
            sim_time=750000
            ;;  
        "RS-32_28_8_dec_mesh_4x4_stp")
            sim_time=40000
            ;; 
        "RS-32_28_8_enc_mesh_4x4_stp")
            sim_time=40000
            ;; 
        *)
            echo "Unknown mapfile: ${mapfile}"
            continue
            ;;
    esac

    for ARCH in "SNAKE" "LIGHT"; do
        for network_protocol in "ABP" "ON_OFF_FLOWCONTROL"; do
            echo "Running ${mapfile}.map with buffer_depth=${buffer_depth} in ${ARCH} architecture (sim=${sim_time}, network_protocol=${network_protocol})..."

            ./noxim -config ../tm_examples/01-16-nownoc.yaml \
                -architecture ${ARCH} \
                -buffer ${buffer_depth} \
                -sim ${sim_time} \
                -network_protocol ${network_protocol} \
                -taskmapping ../tm_examples/map_files/${mapfile}.map \
                -taskmappinglog ../tm_log/${mapfile}/${ARCH}/${ARCH}_${network_protocol}_b${buffer_depth}.log > ../tm_log/tmp_output3.log
        done
    done

done
