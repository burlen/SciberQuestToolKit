#!/bin/bash

#
# The vector fields are 3D, but we need their projection
# onto the x-y plane in order to trace streamlines.
# This script creates a set of dummy fields by symlinks
# to the x and y components of the original, and a symlink
# to an array of zeros for the z component. The genzero
# utility is used to make the array of zeros.
#

DATA_PATH=$1
shift 1

if [[ -z "$DATA_PATH" ]]
then
  echo "Usage: $0 /path/to/2d/run." 1>&2
  exit
fi

cd $DATA_PATH

BX=`ls bx_[0-9]*.gda`

for f in `ls bx_[0-9]*.gda` 
do
  STEP=`echo $f | cut -d_ -f2 | cut -d. -f1`
  echo -n "processing $STEP..."
  # B
  ln -s bx_$STEP.gda  bpx_$STEP.gda
  ln -s by_$STEP.gda  bpy_$STEP.gda
  ln -s zeros.gda     bpz_$STEP.gda
  # E
  ln -s ex_$STEP.gda  epx_$STEP.gda
  ln -s ey_$STEP.gda  epy_$STEP.gda
  ln -s zeros.gda     epz_$STEP.gda
  # V
  ln -s vix_$STEP.gda vipx_$STEP.gda
  ln -s viy_$STEP.gda vipy_$STEP.gda
  ln -s zeros.gda     vipz_$STEP.gda
  echo "OK."
done
