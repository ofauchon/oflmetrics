#!/bin/sh
export LD_LIBRARY_PATH=../xPLLib 
../valgrind --tool=memcheck --leak-check=yes ./xplbridge | tee valgrind.log 
