#!/bin/bash

if [ -z "$1" ]
then
  echo "Error \$1 must specify a configuration."
  echo "Avaialable configs:"
  echo "  plieades-intel"
  echo "  linux-debug"
  echo "  linux-release"
  echo "  kraken-gnu"
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

  "kraken-gnu" )
    # eigen => /lustre/scratch/bloring/apps/eigen-2.0.12-gnu
    # pv3 => /nics/c/home/bloring/ParaView/PV3-3.7-gnu
    cmake ../SciVisToolKit/$BRANCH \
    -DCMAKE_C_COMPILER=/opt/cray/xt-asyncpe/3.6/bin/cc \
    -DCMAKE_CXX_COMPILER=/opt/cray/xt-asyncpe/3.6/bin/CC \
    -DCMAKE_LINKER=/opt/cray/xt-asyncpe/3.6/bin/CC \
    -DMPI_INCLUDE_PATH=/opt/cray/mpt/4.0.1/xt/seastar/mpich2-gnu/include \
    -DMPI_LIBRARY=/opt/cray/mpt/4.0.1/xt/seastar/mpich2-gnu/lib/libmpich.a \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/lustre/scratch/bloring/apps/PV-Plugins/SVTK/gnu/R \
    -DEigen_DIR=$EIGEN \
    -DParaView_DIR=$PV3
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
    echo "Error: invalid config name $1."
    ;;
esac








