#!/bin/bash

if [ -z "$1" ]
then
  echo "usage error:"
  echo "$0 ./SmoothBatch ./asym-2d-smooth-missmarple.xml 2d-asym/asym2d.bov ./cuda_test/ 292500 292500"
  exit -1
fi

export COMPUTE_PROFILE=1
export COMPUTE_PROFILE_CSV=1

configs=(gen L1 L2r L2w gld gldinst gst gstinst warp comp DRAM)
for conf in ${configs[*]}
do
  echo "Running $conf"

  export COMPUTE_PROFILE_CONFIG=runConfig/cudaProf_$conf.conf
  export COMPUTE_PROFILE_LOG=cudaProf_%d_$conf.log

  mpiexec.hydra $*
done

