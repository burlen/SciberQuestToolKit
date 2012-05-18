#!/bin/bash


cmake \
    -DCMAKE_C_COMPILER=/opt/apps/intel/11.1/bin/intel64/icc \
    -DCMAKE_CXX_COMPILER=/opt/apps/intel/11.1/bin/intel64/icpc \
    -DCMAKE_LINKER=/opt/apps/intel/11.1/bin/intel64/icpc \
    -DCMAKE_CXX_FLAGS=-Wno-deprecated \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_TESTING=OFF \
    -DPARAVIEW_BUILD_QT_GUI=OFF \
    -DPARAVIEW_USE_MPI=ON \
    -DMPI_COMPILER=/opt/apps/intel11_1/openmpi/1.3.3/bin/mpicxx \
    -DMPI_EXTRA_LIBRARY=/opt/apps/intel11_1/openmpi/1.3.3/lib/libmpi.so\;/opt/apps/intel11_1/openmpi/1.3.3/lib/libopen-rte.so\;/opt/apps/intel11_1/openmpi/1.3.3/lib/libopen-pal.so\;/usr/lib64/libdl.so\;/usr/lib64/libnsl.so\;/usr/lib64/libutil.so \
    -DMPI_INCLUDE_PATH=/opt/apps/intel11_1/openmpi/1.3.3/include \
    -DMPI_LIBRARY=/opt/apps/intel11_1/openmpi/1.3.3/lib/libmpi.so \
    -DMPI_LINK_FLAGS=-Wl,--export-dynamic \
    -DPARAVIEW_DISABLE_VTK_TESTING=ON \
    -DPARAVIEW_TESTING_WITH_PYTHON=OFF \
    -DBOOST_ROOT=/home/01237/bloring/ParaView/boost_1_46_1 \
    -DVTK_USE_BOOST=ON \
    -DPARAVIEW_ENABLE_PYTHON=ON \
    -DPARAVIEW_USE_MPI=ON \
    -DPARAVIEW_BUILD_PLUGIN=EyeDomeLighting \
    -DPARAVIEW_USE_VISITBRIDGE=ON \
    -DVISIT_BUILD_READER_CGNS=OFF \
    -DVISIT_BUILD_READER_Silo=OFF \
    $* 
