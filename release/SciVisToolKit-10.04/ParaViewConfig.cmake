# +---------------------------------------------------------------------------+
# |                                                                           |
# |                          Locate ParaView build                            |
# |                                                                           |
# +---------------------------------------------------------------------------+
#ParaView3
set(ParaView_DIR 
  ${CMAKE_BINARY_DIR}/PV3-3.7
  CACHE FILEPATH
  "Path to ParaView build.")

if (NOT EXISTS ${ParaView_DIR})
  MESSAGE( FATAL_ERROR 
  "Set ParaView_DIR to the path to your local ParaView3 out of source build." )
endif (NOT EXISTS ${ParaView_DIR})

find_package(ParaView REQUIRED)
include(${PARAVIEW_USE_FILE})
