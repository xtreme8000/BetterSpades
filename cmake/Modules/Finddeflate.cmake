# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# Finddeflate
# -------
#
# Finds the libdeflate library
#
# This will define the following variables::
#
#   deflate_FOUND    - True if the system has the deflate library
#
# and also the following imported target:
#
#   deflate::deflate
#

find_package(PkgConfig)
pkg_check_modules(PC_deflate QUIET deflate)
find_path(deflate_INCLUDE_DIR
  NAMES libdeflate.h
  PATHS ${PC_deflate_INCLUDE_DIRS}
  PATH_SUFFIXES deflate
)
find_library(deflate_LIBRARY
  NAMES deflate
  PATHS ${PC_deflate_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(deflate
  FOUND_VAR deflate_FOUND
  REQUIRED_VARS
    deflate_LIBRARY
    deflate_INCLUDE_DIR
)

if (deflate_FOUND)
  add_library(deflate::deflate STATIC IMPORTED)
  set_target_properties(deflate::deflate PROPERTIES 
    IMPORTED_LOCATION ${deflate_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES  ${deflate_INCLUDE_DIR}
  )
endif (deflate_FOUND)