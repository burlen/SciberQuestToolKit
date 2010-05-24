# +---------------------------------------------------------------------------+
# |                                                                           |
# |                          Locate Eigen Install                             |
# |                                                                           |
# +---------------------------------------------------------------------------+

set(Eigen_DIR 
  ${CMAKE_CURRENT_SOURCE_DIR}/eigen-2.0.12/
  CACHE FILEPATH
  "Path to Eigen 2 install.")

if (NOT EXISTS ${Eigen_DIR})
  MESSAGE(FATAL_ERROR
  "Set Eigen_DIR to the path to your Eigen install." )
endif (NOT EXISTS ${Eigen_DIR})

include_directories(${Eigen_DIR}/include/eigen2/)
