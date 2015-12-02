# - Try to find the hd library
# Once done this will define
#
#  HD_FOUND - system has the hd library
#  HD_INCLUDE_DIR - the hd include directory
#  HD_LIBRARY - Link this to use the hd library
#
# Copyright (C) 2008, Pino Toscano, <pino@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (HD_LIBRARY AND HD_INCLUDE_DIR)
  # in cache already
  set(HD_FOUND TRUE)
else (HD_LIBRARY AND HD_INCLUDE_DIR)

  find_path(HD_INCLUDE_DIR hd.h
  )

  find_library(HD_LIBRARY
    NAMES hd
  )

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(HD DEFAULT_MSG HD_LIBRARY HD_INCLUDE_DIR)
  # ensure that they are cached
  set(HD_INCLUDE_DIR ${HD_INCLUDE_DIR} CACHE INTERNAL "The hd include path")
  set(HD_LIBRARY ${HD_LIBRARY} CACHE INTERNAL "The libraries needed to use hd")

endif (HD_LIBRARY AND HD_INCLUDE_DIR)
