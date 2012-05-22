# +---------------------------------------------------------------------------+
# |                                                                           |
# |                                  MPICH                                    |
# |                                                                           |
# +---------------------------------------------------------------------------+

set(SQTK_WITHOUT_MPI OFF CACHE BOOL "Build without mpi.")

if (SQTK_WITHOUT_MPI)
    message(STATUS "Building without MPI.")
    add_definitions(-DSQTK_WITHOUT_MPI)
else()
  include(FindMPI)
  if (NOT MPI_FOUND)
    message(SEND_ERROR "MPI Is required.")
  else()
    message(STATUS "Building with MPI.")
    add_definitions("-DMPICH_IGNORE_CXX_SEEK")
    include_directories(${MPI_INCLUDE_PATH})
  endif()
endif()
