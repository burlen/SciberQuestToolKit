#!/bin/bash

# (C) 2010 SciberQuest Inc.

if [ $# != 11 ]
then
  echo "Usage: start_pvserver.sh NCPUS WALLTIME ACCOUNT PORT"
  echo
  echo "  EXE          - program to execute."
  echo "  NCPUS        - number of processes in mutiple of 8."
  echo "  MEM          - ram per process. each 8 cpus comes with 32GB ram."
  echo "  WALLTIME     - wall time in HH:MM:SS format."
  echo "  ACCOUNT      - your tg account for billing purposes."
  echo "  QUEUE        - the queue to use."
  echo "  CONFIG_FILE  - xml run configuration"
  echo "  BOV_FILE     - bov file of dataset to process"
  echo "  OUTPUT_PATH  - path where results are stored"
  echo "  START_TIME   - start time"
  echo "  END_TIME=    - end time"
  echo
  exit
fi

export PV_EXE=$1
NCPUS=$2
export PV_NCPUS=$NCPUS
WALLTIME=$4
ACCOUNT=$5
QUEUE=$6
export CONFIG_FILE=$7
export BOV_FILE=$8
export OUTPUT_PATH=$9
export START_TIME=${10}
export END_TIME=${11}

export PV_PATH=/lustre/scratch/proj/sw/paraview/3.12.0/cnl2.2_gnu4.4.4-so

LD_LIBRARY_PATH=/opt/xt-pe/2.2.74/lib:/opt/cray/mpt/5.2.3/xt/seastar/mpich2-gnu/lib:/opt/cray/mpt/5.2.3/xt/seastar/sma/lib64:/opt/gcc/mpc/0.8.1/lib:/opt/gcc/mpfr/2.4.2/lib:/opt/gcc/gmp/4.3.2/lib:/opt/gcc/4.5.3/snos/lib64:/opt/cray/MySQL/5.0.64-1.0202.2899.21.1/lib64/mysql:/opt/cray/MySQL/5.0.64-1.0202.2899.21.1/lib64:/opt/cray/projdb/1.0.0-1.0202.19483.52.1/lib64:/opt/cray/csa/3.0.0-1_2.0202.21426.77.7/lib64:/opt/cray/job/1.5.5-0.1_2.0202.21413.56.7/lib64:/opt/torque/2.4.14/lib
export LD_LIBRARY_PATH=$PV_PATH/lib/paraview-3.12/:$PV_PATH/lib

PATH=/sw/xt/cmake/2.8.2/sles10.1_gnu4.1.2/bin:/sw/xt-cle2.2/subversion/1.4.6/sles10.1_gnu4.3.2/bin:/opt/cray/xt-asyncpe/4.9/bin:/opt/cray/pmi/2.1.4-1.0000.8596.15.1.ss/bin:/opt/gcc/4.5.3/bin:/opt/cray/MySQL/5.0.64-1.0202.2899.21.1/sbin:/opt/cray/MySQL/5.0.64-1.0202.2899.21.1/bin:/opt/cray/projdb/1.0.0-1.0202.19483.52.1/bin:/opt/cray/account/1.0.0-2.0202.19482.49.18/bin:/opt/cray/csa/3.0.0-1_2.0202.21426.77.7/sbin:/opt/cray/csa/3.0.0-1_2.0202.21426.77.7/bin:/opt/cray/job/1.5.5-0.1_2.0202.21413.56.7/bin:/opt/xt-lustre-ss/2.2.74_1.6.5/usr/sbin:/opt/xt-lustre-ss/2.2.74_1.6.5/usr/bin:/opt/xt-boot/2.2.74/bin/snos64:/opt/xt-os/2.2.74/bin/snos64:/opt/xt-service/2.2.74/bin/snos64:/opt/xt-prgenv/2.2.74/bin:/sw/altd/bin:/sw/xt/tgusage/3.0-r2/binary/bin:/sw/xt/bin:/usr/local/gold/bin:/usr/local/hsi/bin:/usr/local/openssh/bin:/opt/moab/5.4.3.s16991/bin:/opt/torque/2.4.14/bin:/opt/modules/3.1.6.5/bin:/usr/local/bin:/usr/bin:/usr/X11R6/bin:/bin:/usr/games:/opt/gnome/bin:/usr/lib/jvm/jre/bin:/usr/lib/mit/bin:/usr/lib/mit/sbin:.:/usr/lib/qt3/bin:/nics/c/home/bloring/bin
export PATH=$PV_PATH/lib/paraview-3.12:$PATH

JID=`qsub -v PV_PATH,PATH,LD_LIBRARY_PATH,PV_NCPUS,CONFIG_FILE,BOV_FILE,OUTPUT_PATH,START_TIME,END_TIME,PV_EXE -N svtk-batch -j eo -A $ACCOUNT -q $QUEUE -l size=$NCPUS,walltime=$WALLTIME $PV_PATH/bin/qsub-svtk-batch-kraken.qsub`
ERRNO=$?
if [ $ERRNO == 0 ] 
then
echo "Job $JID submitted succesfully."
else
echo "ERROR $ERRNO: in job submission."
fi




#!/bin/bash

echo
echo "Starting svtk-batch..."
echo 




