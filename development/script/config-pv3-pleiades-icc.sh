#!/bin/bash

cmake \
  -DCMAKE_C_COMPILER=/nasa/intel/cce/10.1.021/bin/icc \
  -DCMAKE_CXX_COMPILER=/nasa/intel/cce/10.1.021/bin/icpc \
  -DCMAKE_LINKER=/nasa/intel/cce/10.1.021/bin/icpc \
  -DBUILD_SHARED_LIBS=ON \
  -DBUILD_TESTING=OFF \
  -DPARAVIEW_BUILD_QT_GUI=OFF \
  -DPARAVIEW_USE_MPI=ON \
  -DMPI_INCLUDE_PATH=/nasa/sgi/mpt/1.23.0.0/include \
  -DMPI_LIBRARY=/nasa/sgi/mpt/1.23.0.0/lib64/libmpi.so \
  -DVTK_OPENGL_HAS_OSMESA=ON \
  -DOPENGL_INCLUDE_DIR=/u/burlen/apps/Mesa-7.5.1-Intel/include \
  -DOPENGL_gl_LIBRARY=/u/burlen/apps/Mesa-7.5.1-Intel/lib64/libGL.so \
  -DOPENGL_glu_LIBRARY=/u/burlen/apps/Mesa-7.5.1-Intel/lib64/libGLU.so \
  -DOPENGL_xmesa_INCLUDE_DIR=/u/burlen/apps/Mesa-7.5.1-Intel/include \
  -DOSMESA_INCLUDE_DIR=/u/burlen/apps/Mesa-7.5.1-Intel/include \
  -DOSMESA_LIBRARY=/u/burlen/apps/Mesa-7.5.1-Intel/lib64/libOSMesa.so \
  $*


