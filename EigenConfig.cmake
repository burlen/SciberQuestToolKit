# +---------------------------------------------------------------------------+
# |                                                                           |
# |                          Locate Eigen Install                             |
# |                                                                           |
# +---------------------------------------------------------------------------+

set(Eigen_DIR 
  /path/to/eigen
  CACHE FILEPATH
  "Path to ParaView build.")

if (NOT EXISTS ${Eigen_DIR})
  MESSAGE(FATAL_ERROR
  "Set Eigen_DIR to the path to your Eigen install." )
endif (NOT EXISTS ${Eigen_DIR})

include_directories(${Eigen_DIR}/include/eigen2/)
