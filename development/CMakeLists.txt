cmake_minimum_required(VERSION 2.4)
cmake_policy(SET CMP0003 NEW)
#    ____    _ __           ____               __    ____
#   / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
#  _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
# /___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
#
# Copyright 2008 SciberQuest Inc.
#
# +---------------------------------------------------------------------------+
# |                                                                           |
# |                            SciVis Tool Kit                                |
# |                                                                           |
# +---------------------------------------------------------------------------+
#
project(SciVisToolKit)

# Prevent in-source build
if("${PROJECT_SOURCE_DIR}"  STREQUAL "${PROJECT_BINARY_DIR}")
  message(FATAL_ERROR "SciVisToolKit requires an out of source Build.")
endif("${PROJECT_SOURCE_DIR}"  STREQUAL "${PROJECT_BINARY_DIR}")

# Locate our PV build/development install.
include(ParaViewConfig.cmake)
include(EigenConfig.cmake)
include(MPICHConfig.cmake)
include(StateFileConfig.cmake)

# Server manager wrapped sources.
set(SM_SOURCES
  vtkSQBOVReader.cxx
  vtkSQFieldTracer.cxx
  vtkSQHemisphereSource.cxx
  vtkSQLineSource.cxx
  vtkSQPlaneSource.cxx
  vtkSQPointSource.cxx
  vtkSQRandomCells.cxx
  vtkSQRandomSeedPoints.cxx
  vtkSQSeedPointLatice.cxx
  vtkSQSphereSource.cxx
  vtkSQSurfaceVectors.cxx
  vtkSQVolumeSource.cxx
  vtkSQVortexFilter.cxx
  )

# Un-wrapped sources
set(_SOURCES
  BOVMetaData.cxx
  BOVReader.cxx
  BOVTimeStepImage.cxx
  BOVScalarImage.cxx
  BOVVectorImage.cxx
  CartesianBounds.cxx
  CartesianDecomp.cxx
  CartesianDataBlock.cxx
  CartesianDataBlockIODescriptor.cxx
  CartesianDataBlockIODescriptorIterator.cxx
  CartesianExtent.cxx
  CellCopier.cxx
  IdBlock.cxx
  FieldLine.cxx
  FieldTraceData.cxx
  FsUtils.cxx
  GDAMetaData.cxx
  GDAMetaDataKeys.cxx
  IntersectionSet.cxx
  PolyDataCellCopier.cxx
  PolyDataFieldTopologyMap.cxx
  PoincareMapData.cxx
  postream.cxx
  PrintUtils.cxx
  RefCountedPointer.cxx
  StreamlineData.cxx
  TerminationCondition.cxx
  UnstructuredFieldTopologyMap.cxx
  UnstructuredGridCellCopier.cxx
  vtkSQMetaDataKeys.cxx
  vtkSQOOCReader.cxx
  vtkSQOOCBOVReader.cxx
  vtkSQHemisphereSourceConfigurationWriter.cxx
  vtkSQHemisphereSourceConfigurationReader.cxx
  vtkSQPlaneSourceConfigurationWriter.cxx
  vtkSQPlaneSourceConfigurationReader.cxx
  vtkSQVolumeSourceConfigurationWriter.cxx
  vtkSQVolumeSourceConfigurationReader.cxx
  )

set_source_files_properties(
  vtkSQOOCReader.cxx
  ABSTRACT
  )

if (WIN32)
  # Augment windows with some posix codes.
  set(_SOURCES ${_SOURCES} win_windirent.cxx)
endif (WIN32)

if (NOT WIN32)
  # process monitor currently has some unix only features
  message(STATUS "Building process monitor.")
  set(SM_SOURCES ${SM_SOURCES} vtkSQProcessMonitor.cxx)
endif (NOT WIN32)


if(PARAVIEW_BUILD_QT_GUI)
message(STATUS "Including custom panels.")
set(_IFACES)
set(_IFACE_SRCS)
# Qt custom panels
QT4_WRAP_CPP(MOC_SRCS pqSQHemisphereSource.h)
QT4_WRAP_UI(UI_SRCS pqSQHemisphereSourceForm.ui)
ADD_PARAVIEW_OBJECT_PANEL(
  IFACES IFACE_SRCS
  CLASS_NAME pqSQHemisphereSource
  XML_NAME vtkSQHemisphereSource
  XML_GROUP sources)
set(_IFACES ${_IFACES} ${IFACES})
set(_IFACE_SRCS ${_IFACE_SRCS} ${IFACE_SRCS})

QT4_WRAP_CPP(MOC_SRCS pqSQPlaneSource.h)
QT4_WRAP_UI(UI_SRCS pqSQPlaneSourceForm.ui)
ADD_PARAVIEW_OBJECT_PANEL(
  IFACES IFACE_SRCS
  CLASS_NAME pqSQPlaneSource
  XML_NAME vtkSQPlaneSource
  XML_GROUP sources)
set(_IFACES ${_IFACES} ${IFACES})
set(_IFACE_SRCS ${_IFACE_SRCS} ${IFACE_SRCS})

QT4_WRAP_CPP(MOC_SRCS pqSQBOVMetaReader.h)
ADD_PARAVIEW_OBJECT_PANEL(
  IFACES IFACE_SRCS
  CLASS_NAME pqSQBOVMetaReader
  XML_NAME vtkSQBOVMetaReader
  XML_GROUP sources)
set(_IFACES ${_IFACES} ${IFACES})
set(_IFACE_SRCS ${_IFACE_SRCS} ${IFACE_SRCS})

QT4_WRAP_CPP(MOC_SRCS pqSQFieldTracer.h)
ADD_PARAVIEW_OBJECT_PANEL(
  IFACES IFACE_SRCS
  CLASS_NAME pqSQFieldTracer
  XML_NAME vtkSQFieldTracer vtkSQFieldTracerRK45 vtkSQFieldTopologyMapper
  XML_GROUP filters)
set(_IFACES ${_IFACES} ${IFACES})
set(_IFACE_SRCS ${_IFACE_SRCS} ${IFACE_SRCS})

QT4_WRAP_CPP(MOC_SRCS pqSQVolumeSource.h)
QT4_WRAP_UI(UI_SRCS pqSQVolumeSourceForm.ui)
ADD_PARAVIEW_OBJECT_PANEL(
  IFACES IFACE_SRCS
  CLASS_NAME pqSQVolumeSource
  XML_NAME vtkSQVolumeSource
  XML_GROUP sources)
set(_IFACES ${_IFACES} ${IFACES})
set(_IFACE_SRCS ${_IFACE_SRCS} ${IFACE_SRCS})

if (NOT WIN32)
  message(STATUS "Building process monitor ui.")
  QT4_WRAP_CPP(MOC_SRCS pqSQProcessMonitor.h)
  QT4_WRAP_UI(UI_SRCS pqSQProcessMonitorForm.ui)
  ADD_PARAVIEW_OBJECT_PANEL(
    IFACES IFACE_SRCS
    CLASS_NAME pqSQProcessMonitor
    XML_NAME vtkSQProcessMonitor
    XML_GROUP sources)
  set(_IFACES ${_IFACES} ${IFACES})
  set(_IFACE_SRCS ${_IFACE_SRCS} ${IFACE_SRCS})
endif (NOT WIN32)

set (_SOURCES
  ${_SOURCES}
  ${MOC_SRCS}
  ${UI_SRCS}
  ${_IFACE_SRCS}
  pqSQBOVMetaReader.cxx
  pqSQFieldTracer.cxx
  pqSQHemisphereSource.cxx
  pqSQPlaneSource.cxx
  pqSQVolumeSource.cxx
  )

if (NOT WIN32)
  # process monitor currently has some unix only features
  set (_SOURCES ${_SOURCES} pqSQProcessMonitor.cxx)
endif (NOT WIN32)

endif (PARAVIEW_BUILD_QT_GUI)

set (CLIENT_XML
  SciVisToolKitClientReaders.xml
  SciVisToolKitClientSources.xml
  SciVisToolKitClientFilters.xml)

# Generate the plugin, servermanager and wrapped language bindings.
add_paraview_plugin(
  SciVisToolKit "1.0"
  GUI_INTERFACES ${_IFACES}
  SERVER_MANAGER_SOURCES ${SM_SOURCES}
  SERVER_MANAGER_XML SciVisToolKitServerManager.xml
  GUI_RESOURCE_FILES ${CLIENT_XML}
  SOURCES ${_SOURCES} )

install(TARGETS SciVisToolKit DESTINATION ${CMAKE_INSTALL_PREFIX})

#target_link_libraries(SciVisToolKit)

# command line utilities
if (UNIX)
set(BUILD_SVTK_UTILITIES OFF CACHE BOOL "Build SciVisToolKit utilities.")
  if (BUILD_SVTK_UTILITIES)
    message(STATUS "Building SciVisToolKit utilities.")
    include(FindMPI)
    if (NOT MPI_FOUND)
      message(SEND_ERROR "MPI is required for the gentp utility.")
    endif(NOT MPI_FOUND)
    add_definitions("-DMPICH_IGNORE_CXX_SEEK")

    add_executable(gentp gentp.cpp FsUtils.cxx)
    target_link_libraries(gentp ${MPI_LIBRARIES})
    install(TARGETS gentp DESTINATION ${CMAKE_INSTALL_PREFIX})

    add_executable(genzero genzero.cpp)
    install(TARGETS genzero DESTINATION ${CMAKE_INSTALL_PREFIX})

    add_executable(MagnetosphereTopoBatch MagnetosphereTopoBatch.cpp)
    target_link_libraries(MagnetosphereTopoBatch ${MPI_LIBRARIES} SciVisToolKit)
    install(TARGETS MagnetosphereTopoBatch DESTINATION ${CMAKE_INSTALL_PREFIX})

  endif (BUILD_SVTK_UTILITIES)
endif (UNIX)

# testing
set(BUILD_SVTK_TESTING OFF CACHE BOOL "Build project tests.")
if (BUILD_SVTK_TESTING)
  message(STATUS "Including SciVisToolKit tests.")
  include(FindMPI)
  if (NOT MPI_FOUND)
    message(SEND_ERROR "MPI is required for the gentp utility.")
  endif(NOT MPI_FOUND)
  add_definitions("-DMPICH_IGNORE_CXX_SEEK")
  add_executable(TestFieldTopologyMapper TestFieldTopologyMapper.cpp)
  target_link_libraries(TestFieldTopologyMapper SciVisToolKit ${MPI_LIBRARIES})
  install(TARGETS TestFieldTopologyMapper DESTINATION ${CMAKE_INSTALL_PREFIX})
endif (BUILD_SVTK_TESTING)

# instrument -- command line utility
set(BUILD_INSTRUMENT OFF CACHE BOOL "Build intrumentation utility to intrument cxx classes.")
if (BUILD_INSTRUMENT)
  message(STATUS "Including InstrumentClass.")
  add_executable(instrumentClass InstrumentClass.cpp FsUtils.cxx)
  install(TARGETS instrumentClass DESTINATION ${CMAKE_INSTALL_PREFIX})
endif (BUILD_INSTRUMENT)

