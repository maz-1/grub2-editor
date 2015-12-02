# - Try to find QAPT
# Once done this will define
#
#  QAPT_FOUND - system has QApt
#  QAPT_INCLUDE_DIR - the QApt include directory
#  QAPT_LIBRARIES - Link these to use all QApt libs
#  QAPT_DEFINITIONS - Compiler switches required for using QApt

# Copyright (c) 2009, Dario Freddi, <drf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (QAPT_INCLUDE_DIR AND QAPT_LIB)
    set(QAPT_FIND_QUIETLY TRUE)
endif (QAPT_INCLUDE_DIR AND QAPT_LIB)

if (NOT QAPT_MIN_VERSION)
  set(QAPT_MIN_VERSION "0.1.0")
endif (NOT QAPT_MIN_VERSION)

if (NOT WIN32)
   # use pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   find_package(PkgConfig REQUIRED)
   pkg_check_modules(PC_QAPT libqapt)
   if(${PC_QAPT_FOUND})
       set(QAPT_DEFINITIONS ${PC_QAPT_CFLAGS_OTHER})
       string(REGEX MATCH "^[0-9]+" QAPT_VERSION_MAJOR ${PC_QAPT_VERSION})
   endif(${PC_QAPT_FOUND})
endif (NOT WIN32)

find_path( QAPT_INCLUDE_DIR
     NAMES libqapt/qaptversion.h
)

set(QAPT_VERSION_OK TRUE)
if(QAPT_INCLUDE_DIR)
  file(READ ${QAPT_INCLUDE_DIR}/libqapt/qaptversion.h QAPT_VERSION_CONTENT)
  string (REGEX MATCH "QAPT_VERSION_STRING \".*\"\n" QAPT_VERSION_MATCH "${QAPT_VERSION_CONTENT}")

  if(QAPT_VERSION_MATCH)
    string(REGEX REPLACE "QAPT_VERSION_STRING \"(.*)\"\n" "\\1" QAPT_VERSION ${QAPT_VERSION_MATCH})
    if(QAPT_VERSION STRLESS "${QAPT_MIN_VERSION}")
      set(QAPT_VERSION_OK FALSE)
      if(QAPT_FIND_REQUIRED)
        message(FATAL_ERROR "QApt version ${QAPT_VERSION} was found, but it is too old. Please install ${QAPT_MIN_VERSION} or
newer.")
      else(QAPT_FIND_REQUIRED)
        message(STATUS "QApt version ${QAPT_VERSION} is too old. Please install ${QAPT_MIN_VERSION} or newer.")
      endif(QAPT_FIND_REQUIRED)
    endif(QAPT_VERSION STRLESS "${QAPT_MIN_VERSION}")
  endif(QAPT_VERSION_MATCH)
elseif(QAPT_INCLUDE_DIR)
  # The version is so old that it does not even have the file
  set(QAPT_VERSION_OK FALSE)
  if(QAPT_FIND_REQUIRED)
    message(FATAL_ERROR "It looks like QApt is too old. Please install QApt version ${QAPT_MIN_VERSION} or newer.")
  else(QAPT_FIND_REQUIRED)
    message(STATUS "It looks like QApt is too old. Please install QApt version ${QAPT_MIN_VERSION} or newer.")
  endif(QAPT_FIND_REQUIRED)
endif(QAPT_INCLUDE_DIR)

    find_library(QAPT_LIBRARY
        NAMES qapt
        HINTS ${PC_QAPT_LIBDIR} ${PC_QAPT_LIBRARY_DIRS}
        )

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set QAPT_FOUND to TRUE if
# all listed variables are TRUE
find_package_handle_standard_args(QAPT DEFAULT_MSG QAPT_LIBRARY QAPT_INCLUDE_DIR QAPT_VERSION_OK)

mark_as_advanced(QAPT_INCLUDE_DIR QAPT_LIBRARY QAPT_VERSION_OK)

