#!/bin/bash

for file in ../tm_examples/*.map; do 
 file=${file##*/}; 
 file=${file%.*}; 
 ../bin/noxim -power ../bin/power.yaml -config ../tm_examples/01-16-nownoc.yaml -taskmapping ../tm_examples/${file}.map -taskmappinglog ../tm_log/${file}.log; 
 sleep 0.3
done;
