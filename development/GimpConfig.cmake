#    ____    _ __           ____               __    ____
#   / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
#  _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
# /___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
#
# Copyright 2008 SciberQuest Inc.
#
find_program(GIMP_EXE "gimp" DOC "/path/to/gimp")
mark_as_advanced(GIMP_EXE)
if (GIMP_EXE)
  set(GIMP_FOUND TRUE)
  execute_process(COMMAND ${GIMP_EXE} "--version" OUTPUT_VARIABLE GIMP_VERSION)
  string(TOLOWER ${GIMP_VERSION} GIMP_VERSION)
  string(REGEX MATCH "[0-9]\\.[0-9]" GIMP_VERSION ${GIMP_VERSION})
  if (WIN32)
    set(GIMP_SCRIPT_PATH
      "$ENV{UserProfile}/.gimp-${GIMP_VERSION}/scripts/"
      CACHE PATH "Where to install gimp scripts."
      )
  else ()
    set(GIMP_SCRIPT_PATH
      "$ENV{HOME}/.gimp-${GIMP_VERSION}/scripts/"
      CACHE PATH "Where to install gimp scripts."
      )
  endif()
  message(STATUS "${GIMP_EXE} ${GIMP_VERSION} found.")
  message(STATUS "GIMP_SCRIPT_PATH=${GIMP_SCRIPT_PATH}")
else()
  set(GIMP_FOUND FALSE)
  set(GIMP_VERSION "")
  set(GIMP_EXE "")
  set(GIMP_SCRIPT_PATH "")
  #message(STATUS "GIMP executable was not found.")
endif()
