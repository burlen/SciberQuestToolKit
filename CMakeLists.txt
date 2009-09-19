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
# Plugin that reads "raw" files, in parallel, handles time series and
# vector data.
#
project(SciVisToolKit)

# Locate our PV build/development install.
include(ParaViewConfig.cmake)

# Server manager sources.
set(SM_SOURCES
  vtkBOVReader.cxx
  vtkFieldTracer.cxx
  vtkPFieldTracer.cxx
  vtkFieldTopologyAnalyzer.cxx
  vtkDistributedFieldTracer.cxx
  vtkFaceCenteredDivergence.cxx)

# Sources which aren't for use by the server manager.
set(_SOURCES
  BOVReader.cxx
  GDAMetaData.cxx
  PrintUtils.cxx
  vtkFieldTopologyAnalyzerUtils.cxx)

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

# Generate the plugin, servermanager and wrapped language bindings.
add_paraview_plugin(
  SciVisToolKit "1.0"
  SERVER_MANAGER_SOURCES ${SM_SOURCES}
  SERVER_MANAGER_XML SciVisToolKitServerManager.xml
  GUI_RESOURCE_FILES SciVisToolKitClient.xml
  SOURCES ${_SOURCES})

install(TARGETS SciVisToolKit DESTINATION ${CMAKE_INSTALL_PREFIX})
