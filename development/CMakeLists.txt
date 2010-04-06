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

# Locate our PV build/development install.
include(ParaViewConfig.cmake)
include(EigenConfig.cmake)
include(MPICHConfig.cmake)

# Server manager wrapped sources.
set(SM_SOURCES
  vtkBOVReader.cxx
  vtkSQVortexFilter.cxx
  vtkOOCFieldTracer.cxx
  vtkOOCDFieldTracer.cxx
  vtkSQPlaneSource.cxx
  vtkSQSphereSource.cxx
  vtkSQHemisphereSource.cxx
  )

# Un-wrapped sources
set(_SOURCES
  BOVReader.cxx
  BOVTimeStepImage.cxx
  BOVScalarImage.cxx
  BOVVectorImage.cxx
  FieldTopologyMap.cxx
  FsUtils.cxx
  GDAMetaData.cxx
  GDAMetaDataKeys.cxx
  IntersectionSet.cxx
  PolyDataFieldTopologyMap.cxx
  PrintUtils.cxx
  TerminationCondition.cxx
  TracerFieldTopologyMap.cxx
  UnstructuredFieldTopologyMap.cxx
  vtkMetaDataKeys.cxx
  vtkOOCReader.cxx
  vtkOOCBOVReader.cxx
  vtkSQHemisphereSourceConfigurationWriter.cxx
  vtkSQHemisphereSourceConfigurationReader.cxx
  vtkSQPlaneSourceConfigurationWriter.cxx
  vtkSQPlaneSourceConfigurationReader.cxx
  WorkQueue.cxx
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkOOCReader.cxx
  ABSTRACT
  )

if (WIN32)
  # Augment windows with some posix codes.
  set(_SOURCES ${_SOURCES} win_windirent.cxx)
endif (WIN32)

if (UNIX)
  # process monitor currently has some unix only features
  set(SM_SOURCES ${SM_SOURCES} vtkProcessMonitor.cxx)
endif (UNIX)


# Verison dependent sources.
set(PV_VER ${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR})
if (PV_VER LESS 3.6)
  message(STATUS "-- Using ParaView 3.7 classes in build for ParaView ${PV_VER}.")
  set(_SOURCES 
    ${_SOURCES} 
    vtkAMRBox_3.7.cxx
    vtkInterpolatedVelocityField-3.7.cxx)
  add_definitions(-DPV_3_4_BUILD)
endif (PV_VER LESS 3.6)


if(PARAVIEW_BUILD_QT_GUI)
message(STATUS "-- Including custom panels.")
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

if (UNIX)
  QT4_WRAP_CPP(MOC_SRCS pqProcessMonitor.h)
  QT4_WRAP_UI(UI_SRCS pqProcessMonitorForm.ui)
  ADD_PARAVIEW_OBJECT_PANEL(
    IFACES IFACE_SRCS
    CLASS_NAME pqProcessMonitor
    XML_NAME vtkProcessMonitor
    XML_GROUP sources)
  set(_IFACES ${_IFACES} ${IFACES})
  set(_IFACE_SRCS ${_IFACE_SRCS} ${IFACE_SRCS})
endif (UNIX)

set (_SOURCES
  ${_SOURCES}
  ${MOC_SRCS}
  ${UI_SRCS}
  ${_IFACE_SRCS}
  pqSQHemisphereSource.cxx
  pqSQPlaneSource.cxx
  )

if (UNIX)
  # process monitor currently has some unix only features
  set (_SOURCES ${_SOURCES} pqProcessMonitor.cxx)
endif (UNIX)

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

target_link_libraries(SciVisToolKit)

# gentp -- command line utility
if (UNIX)
set(BUILD_GENTP OFF CACHE BOOL "Build gentp the utility to generate temperature and pressure arrays.")
  if (BUILD_GENTP)
    message(STATUS "-- Including gentp.")
    include(FindMPI)
    if (NOT MPI_FOUND)
      message(SEND_ERROR "MPI is required for the gentp utility.")
    endif(NOT MPI_FOUND)
    add_definitions("-DMPICH_IGNORE_CXX_SEEK")
    add_executable(gentp gentp.cpp FsUtils.cxx)
    target_link_libraries(gentp ${MPI_LIBRARIES})
    install(TARGETS gentp DESTINATION ${CMAKE_INSTALL_PREFIX})
  endif (BUILD_GENTP)
endif (UNIX)

# instrument -- command line utility
set(BUILD_INSTRUMENT OFF CACHE BOOL "Build intrumentation utility to intrument cxx classes.")
if (BUILD_INSTRUMENT)
  message(STATUS "-- Including InstrumentClass.")
  add_executable(instrumentClass InstrumentClass.cpp FsUtils.cxx)
  install(TARGETS instrumentClass DESTINATION ${CMAKE_INSTALL_PREFIX})
endif (BUILD_INSTRUMENT)

