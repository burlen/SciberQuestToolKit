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

# Server manager wrapped sources.
set(SM_SOURCES
  vtkSQBOVReader.cxx
  vtkSQFieldTracer.cxx
  vtkSQHemisphereSource.cxx
  vtkSQSeedPointLatice.cxx
  vtkSQPlaneSource.cxx
  vtkSQRandomSeedPoints.cxx
  vtkSQSphereSource.cxx
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
  FieldLine.cxx
  FieldTraceData.cxx
  FsUtils.cxx
  GDAMetaData.cxx
  GDAMetaDataKeys.cxx
  IntersectionSet.cxx
  PolyDataFieldTopologyMap.cxx
  PoincareMapData.cxx
  PrintUtils.cxx
  RefCountedPointer.cxx
  StreamlineData.cxx
  TerminationCondition.cxx
  UnstructuredFieldTopologyMap.cxx
  vtkSQMetaDataKeys.cxx
  vtkSQOOCReader.cxx
  vtkSQOOCBOVReader.cxx
  vtkSQHemisphereSourceConfigurationWriter.cxx
  vtkSQHemisphereSourceConfigurationReader.cxx
  vtkSQPlaneSourceConfigurationWriter.cxx
  vtkSQPlaneSourceConfigurationReader.cxx
  WorkQueue.cxx
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkSQOOCReader.cxx
  ABSTRACT
  )

if (WIN32)
  # Augment windows with some posix codes.
  set(_SOURCES ${_SOURCES} win_windirent.cxx)
endif (WIN32)

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

set (_SOURCES
  ${_SOURCES}
  ${MOC_SRCS}
  ${UI_SRCS}
  ${_IFACE_SRCS}
  pqSQHemisphereSource.cxx
  pqSQPlaneSource.cxx
  )

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

