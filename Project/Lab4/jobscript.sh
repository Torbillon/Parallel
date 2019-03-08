#!/bin/bash
#SBATCH --job-name="mpimmrunsh"
#SBATCH --output="mpimmrunsh.%j.%N.out"
#SBATCH --partition=debug
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=24
#SBATCH --export=ALL
#SBATCH -t 00:05:00
export MV2_SHOW_CPU_BINDING = 1
ibrun - np 8 ./mpi_mm
