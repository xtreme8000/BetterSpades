# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# Findglfw3
# -------
#
# Finds the glfw3 library
#
# This will define the following variables::
#
#   glfw3_FOUND    - True if the system has the glfw3 library
#
# and also the following imported target:
#
#   glfw3::glfw3
#
find_package(PkgConfig)
pkg_check_modules(PC_glfw3 QUIET glfw3)

find_path(glfw3_INCLUDE_DIR
  NAMES GLFW/glfw3.h
  PATHS ${PC_glfw3_INCLUDE_DIRS} ../../deps
  PATH_SUFFIXES GLFW
)
find_library(glfw3_LIBRARY
  NAMES glfw glfw3
  PATHS ${PC_glfw3_LIBRARY_DIRS} ../../deps
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(glfw3
  FOUND_VAR glfw3_FOUND
  REQUIRED_VARS
    glfw3_LIBRARY
    glfw3_INCLUDE_DIR
)

if (glfw3_FOUND)
  add_library(glfw3::glfw3 STATIC IMPORTED)
  set_target_properties(glfw3::glfw3 PROPERTIES
    IMPORTED_LOCATION ${glfw3_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES  ${glfw3_INCLUDE_DIR}
  )
endif (glfw3_FOUND)
