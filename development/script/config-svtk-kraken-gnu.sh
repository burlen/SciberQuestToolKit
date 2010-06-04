#!/bin/bash

cmake  \
  -DCMAKE_C_COMPILER=/opt/cray/xt-asyncpe/3.6/bin/cc \
  -DCMAKE_CXX_COMPILER=/opt/cray/xt-asyncpe/3.6/bin/CC \
  -DCMAKE_LINKER=/opt/cray/xt-asyncpe/3.6/bin/CC \
  -DMPI_INCLUDE_PATH=/opt/cray/mpt/4.0.1/xt/seastar/mpich2-gnu/include \
  -DMPI_LIBRARY=/opt/cray/mpt/4.0.1/xt/seastar/mpich2-gnu/lib/libmpich.a \
  $*
    







