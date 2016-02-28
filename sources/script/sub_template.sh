#!/bin/bash

#              A line beginning with #PBS is a PBS directive.
#              PBS directives must come first; any directives
#                 after the first executable statement are ignored.
#PBS -N %JOB_NAME%

#          Specify the number of nodes requested and the
#          number of processors per node. 
#PBS -l nodes=%NODES%:ppn=%PROCESSES%
#
###PBS -o stdout_file
###PBS -e stderr_file
#          Specify the maximum cpu and wall clock time. The wall
#          clock time should take possible queue waiting time into
#          account.  Format:   hhhh:mm:ss   hours:minutes:seconds
#PBS -l     cput=2:00:00
#PBS -l walltime=4:00:00
#          Specify the maximum amount of physical memory required per process.
#          kb for kilobytes, mb for megabytes, gb for gigabytes.
###PBS -l pmem=512mb
##########################################
#                                        #
#   Output some useful job information.  #
#                                        #
##########################################
NCPU=`wc -l < $PBS_NODEFILE`
# Please set INPUT env variable
INPUT="%INPUTARGS%"

echo ------------------------------------------------------
echo '| This job is allocated on '${NCPU}' cpu(s)'
echo ------------------------------------------------------
echo ""

echo "----- NODEFILE -----"
cat $PBS_NODEFILE
echo "-------------------"
echo ""

echo "----- VARS -----"
echo "PBS = $PBS_NP"

#WDIR=/scratch/$USER.$PBS_JOBID

# Without $HOME output not works
# there are some problem with torque permission on the
# folder scratch
WDIR=$HOME/scratch/$USER.$PBS_JOBID  
cd $WDIR
#cp $HOME/src/hello_world $WDIR	

echo "wdir = "$WDIR
echo "pwd = $(pwd)"
echo "files = $(ls -l)"
echo "----------------"
echo ""

echo "----- OUTPUT -----"
mpirun_rsh -rsh -np $PBS_NP -hostfile $PBS_NODEFILE %EXE_PATH% $INPUT
echo "----------------"
