#!/bin/bash

DATA_PATH=$1
shift 1

if [[ -z "$DATA_PATH" ]]
then
  echo "Usage: $0 /path/to/2d/run." 1>&2
  exit
fi

cd $DATA_PATH

BX=`ls bx_[0-9]*.gda`

for bxf in "$BX"
do
  STEP=`echo $bxf | cut -d_ -f2 | cut -d. -f1`
  ln -s bx_$STEP.gda  bpx_$STEP.gda
  ln -s by_$STEP.gda  bpy_$STEP.gda
  ln -s zeros.gda     bpz_$STEP.gda
  ln -s ex_$STEP.gda  epx_$STEP.gda
  ln -s ey_$STEP.gda  epy_$STEP.gda
  ln -s zeros.gda     epz_$STEP.gda
  ln -s vix_$STEP.gda vipx_$STEP.gda
  ln -s viy_$STEP.gda vipy_$STEP.gda
  ln -s zeros.gda     vipz_$STEP.gda
done
