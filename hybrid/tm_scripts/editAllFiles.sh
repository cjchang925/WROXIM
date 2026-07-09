#!/bin/bash

kate -n ../src/{*.cpp,*.h} ../bin/Makefile ../src/taskMapping/{*.cpp,*.h,Makefile} ../{tm_examples/*,tm_log/*,tm_notes/*} &> /dev/null &
