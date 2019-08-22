# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# Findenet
# -------
#
# Finds the libenet library
#
# This will define the following variables::
#
#   enet_FOUND    - True if the system has the enet library
#
# and also the following imported target:
#
#   enet::enet
#
find_package(PkgConfig)
pkg_check_modules(PC_enet QUIET enet)

find_path(enet_INCLUDE_DIR
  NAMES enet/enet.h
  PATHS ${PC_enet_INCLUDE_DIRS}
  PATH_SUFFIXES enet
)

find_library(enet_LIBRARY
  NAMES enet
  PATHS ${PC_enet_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(enet
  FOUND_VAR enet_FOUND
  REQUIRED_VARS
    enet_LIBRARY
    enet_INCLUDE_DIR
)

if (enet_FOUND)
  add_library(enet::enet STATIC IMPORTED)
  set_target_properties(enet::enet PROPERTIES 
    IMPORTED_LOCATION ${enet_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES ${enet_INCLUDE_DIR}
  )
  target_include_directories(enet::enet INTERFACE ${enet_INCLUDE_DIR})
  if (WIN32)
    set_target_properties(enet::enet PROPERTIES 
      INTERFACE_LINK_LIBRARIES "ws2_32;winmm"
    )
  endif (WIN32)
endif (enet_FOUND)
