#!/bin/bash

BRANCH=development

# Config type
if [ -z "$1" ]
then
  echo "Error \$1 must specify a configuration."
  echo "Avaialable configs:"
  echo "  linux-debug"
  echo "  linux-release"
  echo "  plieades-intel"
  echo "  kraken-gnu"
  echo "  kraken-pgi"
  exit
fi
CONFIG=$1

# path to paraview install
PV3=$2
if [ -z "$2" ]
then
  echo "Error set \$2 to /path/to/PV3/Build/Or/Install"
  exit
fi

shift
shift
# what's left of command line is passed on to cmake

case $CONFIG in

  "pleiades-intel" )
    ccmake ../SciVisToolKit/$BRANCH \
    -DCMAKE_C_COMPILER=/nasa/intel/Compiler/11.0/083/bin/intel64/icc \
    -DCMAKE_CXX_COMPILER=/nasa/intel/Compiler/11.0/083/bin/intel64/icpc \
    -DCMAKE_LINKER=/nasa/intel/Compiler/11.0/083/bin/intel64/icpc \
    -DMPI_INCLUDE_PATH=/nasa/sgi/mpt/1.25/include \
    -DMPI_LIBRARY=/nasa/sgi/mpt/1.25/lib/libmpi.so \
    -DParaView_DIR=$PV3 \
    -DBUILD_GENTP=ON \
    $*
    ;;

  "kraken-gnu" )
    cmake ../SciVisToolKit/$BRANCH \
    -DCMAKE_C_COMPILER=/opt/cray/xt-asyncpe/3.6/bin/cc \
    -DCMAKE_CXX_COMPILER=/opt/cray/xt-asyncpe/3.6/bin/CC \
    -DCMAKE_LINKER=/opt/cray/xt-asyncpe/3.6/bin/CC \
    -DMPI_INCLUDE_PATH=/opt/cray/mpt/4.0.1/xt/seastar/mpich2-gnu/include \
    -DMPI_LIBRARY=/opt/cray/mpt/4.0.1/xt/seastar/mpich2-gnu/lib/libmpich.a \
    -DParaView_DIR=$PV3 \
    -DBUILD_GENTP=ON \
    $*
    ;;

  "kraken-pgi" )
    cmake ../SciVisToolKit/$BRANCH \
    -DCMAKE_C_COMPILER=/opt/cray/xt-asyncpe/3.6/bin/cc \
    -DCMAKE_CXX_COMPILER=/opt/cray/xt-asyncpe/3.6/bin/CC \
    -DCMAKE_LINKER=/opt/cray/xt-asyncpe/3.6/bin/CC \
    -DMPI_INCLUDE_PATH=/opt/cray/mpt/4.0.1/xt/seastar/mpich2-pgi/include \
    -DMPI_LIBRARY=/opt/cray/mpt/4.0.1/xt/seastar/mpich2-pgi/lib/libmpich.a \
    -DParaView_DIR=$PV3 \
    -DBUILD_GENTP=ON \
    $*
    ;;

  "linux-debug" )
    cmake ../SciVisToolKit/$BRANCH \
    -DCMAKE_CXX_FLAGS="-Wall" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DParaView_DIR=$PV3 \
    $*
    ;;

  "linux-release" )
    cmake ../SciVisToolKit/$BRANCH \
    -DCMAKE_BUILD_TYPE=Release \
    -DParaView_DIR=$PV3 \
    $*
    ;;

  * )
    echo "Error: invalid config name $1."
    ;;
esac








