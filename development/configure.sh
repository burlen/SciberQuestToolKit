#!/bin/bash

if [ -z "$1" ]
then
  echo "Error \$1 must specify a configuration."
  exit
fi

PV3=$2
if [ -z "$2" ]
then
  echo "Error set \$2 to /path/to/PV3/Build/Or/Install"
  exit
fi

EIGEN=$3
if [ -z "$3" ]
then
  echo "Error set \$3 to /path/to/eigen/install"
  exit
fi

if [ -z "$4" ]
then
  echo "\$4 not set selecting development branch."
  BRANCH=development
fi

case $1 in

  "pleiades-intel" )
    ccmake ../SciVisToolKit/$BRANCH \
    -DCMAKE_C_COMPILER=/nasa/intel/Compiler/11.0/083/bin/intel64/icc \
    -DCMAKE_CXX_COMPILER=/nasa/intel/Compiler/11.0/083/bin/intel64/icpc \
    -DCMAKE_LINKER=/nasa/intel/Compiler/11.0/083/bin/intel64/icpc \
    -DBUILD_GENTP=ON \
    -DMPI_INCLUDE_PATH=/nasa/sgi/mpt/1.25/include \
    -DMPI_LIBRARY=/nasa/sgi/mpt/1.25/lib/libmpi.so \
    -DParaView_DIR=$PV3 \
    -DEigen_DIR=$EIGEN
    ;;

  "linux-debug" )
    cmake ../SciVisToolKit/$BRANCH \
    -DCMAKE_BUILD_TYPE=Debug \
    -DParaView_DIR=$PV3 \
    -DEigen_DIR=$EIGEN
    ;;

  "linux-release" )
    cmake ../SciVisToolKit/$BRANCH \
    -DCMAKE_BUILD_TYPE=Release \
    -DParaView_DIR=$PV3 \
    -DEigen_DIR=$EIGEN
    ;;

  * )
    echo "Avaialable configs:"
    echo "  plieades-intel"
    echo "  linux-debug"
    echo "  linux-release"
    ;;
esac








