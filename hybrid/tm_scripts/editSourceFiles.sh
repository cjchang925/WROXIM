#!/bin/bash

kate -n ../src/{*.cpp,*.h} ../bin/Makefile ../src/taskMapping/{*.cpp,*.h,Makefile} &> /dev/null &
