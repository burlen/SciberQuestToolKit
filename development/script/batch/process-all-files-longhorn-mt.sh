#!/bin/bash

# following parameters can be modifed here for different users
ACCT=TG-ATM090046
QUEUE=request
SQTK_INSTALL=/home/01237/bloring/apps/PV3-3.12.0-R-IM

n_args=$#
required_n_args=7

if [ "$n_args" -lt "$required_n_args" ]
then
  echo "n_args=$n_args"
  echo "$@"
  echo "expected the following command line arguments:"
  echo "N_CPUS"
  echo "RUN_TIME"
  echo "EXE"
  echo "SRC_DIR"
  echo "CONFIG_FILE"
  echo "BOV_FILE"
  echo "DEST_DIR"
  exit -1
fi

N_CPUS=$1
RUN_TIME=$2
EXE=$3
SRC_DIR=$4
CONFIG_FILE=$5
BOV_FILE=$6
DEST_DIR=$7

echo "ACCT=$ACCT"
echo "QUEUE=$QUEUE"
echo "SQTK_INSTALL=$SQTK_INSTALL"
echo "N_CPUS=$N_CPUS"
echo "RUN_TIME=$RUN_TIME"
echo "EXE=$EXE"
echo "SRC_DIR=$SRC_DIR"
echo "CONFIG_FILE=$CONFIG_FILE"
echo "BOV_FILE=$BOV_FILE"
echo "DEST_DIR=$DEST_DIR"

for i in `seq 1 $required_n_args`
do
  shift
done

trap "echo exiting; exit -1;" INT TERM

for ifile in `ls  $SRC_DIR/bx\_[0-9]*.gda | xargs -n1 basename`
do
  step=`echo $ifile | cut -d_ -f2 | cut -d. -f1`

  if test -n "$(find $DEST_DIR/`printf %08.f $step`/ -maxdepth 1 -name 'mt.pvt[pu]' -print -quit)";
  then
    echo "skipped $step";
  else
    # only allowed 20 jobs in the queue
    over_limit=1
    while [ "$over_limit" -eq 1 ]
    do
      njobs=`qstat | wc -l`
      if [ "$njobs" -lt 19 ]
      then
        over_limit=0
      fi
    done

    echo "submitted for $ifile $ofile $step"

    #echo \
    qsub -A $ACCT -V -N mt-$step -q $QUEUE -P vis -pe 4way $N_CPUS -l h_rt=$RUN_TIME \
         $SQTK_INSTALL/bin/qsub-sqtk-batch-longhorn.qsub \
         $SQTK_INSTALL/bin/$EXE \
         $CONFIG_FILE \
         $SRC_DIR/$BOV_FILE \
         $DEST_DIR \
         $step \
         $step
  fi
done

