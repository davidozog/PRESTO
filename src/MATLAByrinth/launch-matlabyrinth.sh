#!/bin/bash
#PBS -l nodes=cn50:ppn=1+cn51:ppn=1+cn52:ppn=1+cn53:ppn=1+cn54:ppn=1+cn55:ppn=1+cn56:ppn=1+cn57:ppn=1+cn58:ppn=1+cn59:ppn=1+cn60:ppn=1

# Join the STDOUT and STDERR streams into STDOUT
# PBS -j oe
    
# Write STDOUT to output.txt
#PBS -N matlabyrinth
#PBS -l walltime=24:00:00

export WORK_DIR=/ibrix/home11/ozog/School/Masters-Thesis/src/matlab-python-popen
cd $WORK_DIR
    

# Initialize and clean Environment Modules 
module purge
module load python matlab
module load mpi-tor/openmpi-1.4.5_gcc-4.4.6

mpirun --mca btl_tcp_if_include br2 -np 11 python matlab-subprocess-stdout.py > matlabyrinth.log
