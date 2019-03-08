#!/bin/bash

mpicc -c data_compressor.c -o data_compressor.o

mpicc -c preprocessor_comp.c -o preprocessor_comp.o

mpicc -c file_sys.c -o file_sys.o

mpicc data_compressor.o preprocessor_comp.o file_sys.o -o program

rm data_compressor.o preprocessor_comp.o file_sys.o
