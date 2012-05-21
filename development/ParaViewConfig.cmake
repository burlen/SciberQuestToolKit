# +---------------------------------------------------------------------------+
# |                                                                           |
# |                          Locate ParaView build                            |
# |                                                                           |
# +---------------------------------------------------------------------------+
#ParaView3
set(ParaView_DIR
  /path/to/paraview/build
  CACHE FILEPATH
  "Path to ParaView build.")

if (NOT EXISTS ${ParaView_DIR})
  MESSAGE(FATAL_ERROR
  "Set ParaView_DIR to the path to a ParaView build." )
endif (NOT EXISTS ${ParaView_DIR})

find_package(ParaView REQUIRED)
include(${PARAVIEW_USE_FILE})

message(STATUS "ParaView ${PARAVIEW_VERSION_FULL} found.")

if (NOT PARAVIEW_USE_MPI)
  message(STATUS "WARNING: Building without MPI.")
  message(STATUS "WARNING: You will only be able to use this build by"
  message(STATUS "WARNING: connecting to an MPI enabled instance of pvserver.")
  add_definitions(-DSVTK_WITHOUT_MPI)
endif()

