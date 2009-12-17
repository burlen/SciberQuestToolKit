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

# Server manager sources.
set(SM_SOURCES
  vtkPVImageReader.cxx
  vtkBOVReader.cxx
  vtkFieldTracer.cxx
  vtkPFieldTracer.cxx
  vtkFieldTopologyAnalyzer.cxx
  vtkDistributedFieldTracer.cxx
  vtkFieldAnalysis.cxx
  vtkOOCFieldTracer.cxx
  vtkOOCDFieldTracer.cxx
  vtkSQPlaneSource.cxx
  vtkSQSphereSource.cxx
  vtkSQHemisphereSource.cxx
  vtkProcessMonitor.cxx)

# Sources which aren't for use by the server manager.
set(_SOURCES
  FsUtils.cxx
  vtkOOCReader.cxx
  vtkOOCBOVReader.cxx
  BOVReader.cxx
  GDAMetaData.cxx
  GDAMetaDataKeys.cxx
  PrintUtils.cxx
  IntersectionSet.cxx
  TerminationCondition.cxx
  vtkMetaDataKeys.cxx
  BOVTimeStepImage.cxx
  BOVScalarImage.cxx
  BOVVectorImage.cxx
  WorkQueue.cxx
  FieldTopologyMap.cxx
  PolyDataFieldTopologyMap.cxx
  UnstructuredFieldTopologyMap.cxx
  TracerFieldTopologyMap.cxx)

SET_SOURCE_FILES_PROPERTIES(
vtkOOCReader.cxx
ABSTRACT
)

if (WIN32)
  set(_SOURCES ${_SOURCES} windirent.cxx)
endif (WIN32)

# Verison dependent sources.
set(PV_VER ${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR})
if (PV_VER LESS 3.6)
  message("-- Using ParaView 3.7 classes in build for ParaView ${PV_VER}.")
  set(_SOURCES 
    ${_SOURCES} 
    vtkAMRBox_3.7.cxx
    vtkInterpolatedVelocityField-3.7.cxx)
  add_definitions(-DPV_3_4_BUILD)
endif (PV_VER LESS 3.6)


if(PARAVIEW_BUILD_QT_GUI)
# Qt custom panels
QT4_WRAP_CPP(MOC_SRCS pqProcessMonitor.h)
QT4_WRAP_UI(UI_SRCS pqProcessMonitorForm.ui)
ADD_PARAVIEW_OBJECT_PANEL(
  IFACES IFACE_SRCS
  CLASS_NAME pqProcessMonitor
  XML_NAME vtkProcessMonitor
  XML_GROUP sources)

QT4_WRAP_CPP(MOC_SRCS pqSQPlaneSource.h)
QT4_WRAP_UI(UI_SRCS pqSQPlaneSourceForm.ui)
ADD_PARAVIEW_OBJECT_PANEL(
  IFACES IFACE_SRCS
  CLASS_NAME pqSQPlaneSource
  XML_NAME vtkSQPlaneSource
  XML_GROUP sources)

QT4_WRAP_CPP(MOC_SRCS pqSQHemisphereSource.h)
QT4_WRAP_UI(UI_SRCS pqSQHemisphereSourceForm.ui)
ADD_PARAVIEW_OBJECT_PANEL(
  IFACES IFACE_SRCS
  CLASS_NAME pqSQHemisphereSource
  XML_NAME vtkSQHemisphereSource
  XML_GROUP sources)

set (_SOURCES
  ${_SOURCES}
  ${MOC_SRCS}
  ${UI_SRCS}
  ${IFACE_SRCS}
  pqSQHemisphereSource.cxx
  pqSQPlaneSource.cxx
  pqProcessMonitor.cxx)

endif (PARAVIEW_BUILD_QT_GUI)

# Generate the plugin, servermanager and wrapped language bindings.
add_paraview_plugin(
  SciVisToolKit "1.0"
  GUI_INTERFACES ${IFACES}
  SERVER_MANAGER_SOURCES ${SM_SOURCES}
  SERVER_MANAGER_XML SciVisToolKitServerManager.xml
  GUI_RESOURCE_FILES SciVisToolKitClient.xml
  SOURCES ${_SOURCES} )

install(TARGETS SciVisToolKit DESTINATION ${CMAKE_INSTALL_PREFIX})

# gentp -- command line utility
if (UNIX)
set(BUILD_GENTP OFF CACHE BOOL "Build gentp the utility to generate temperature and pressure arrays.")
  if (BUILD_GENTP)
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
