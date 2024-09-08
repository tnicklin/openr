# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# CMake package configuration file for fbthrift
#
# Defines the target "FBThrift::thriftcpp2"
# Add this to your target_link_libraries() call to depend on thrift.
#
# Note that fbthrift depends on the wangle::wangle target.  This should
# be found by calling find_package(wangle CONFIG) before finding fbthrift.
#
# This also defines the following variables
# FBTHRIFT_COMPILER - The path to the thrift compiler
#


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was FBThriftConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

include(CMakeFindDependencyMacro)

# Save the PACKAGE_PREFIX_DIR in a separate variable.  find_dependency() calls
# below can replace PACKAGE_PREFIX_DIR with the prefix of the dependency that
# was searched for.
set(FBTHRIFT_PREFIX_DIR "${PACKAGE_PREFIX_DIR}")

set_and_check(FBTHRIFT_CMAKE_DIR "${PACKAGE_PREFIX_DIR}/lib/cmake/fbthrift")
set_and_check(FBTHRIFT_INCLUDE_DIR "${PACKAGE_PREFIX_DIR}/include")
if (WIN32)
  set_and_check(FBTHRIFT_COMPILER "${PACKAGE_PREFIX_DIR}/bin/thrift1.exe")
else()
  set_and_check(FBTHRIFT_COMPILER "${PACKAGE_PREFIX_DIR}/bin/thrift1")
endif()

find_dependency(ZLIB REQUIRED)
find_package(mvfst CONFIG REQUIRED)

if (NOT TARGET FBThrift::thriftcpp2)
  include("${FBTHRIFT_CMAKE_DIR}/FBThriftTargets.cmake")
endif()

#
# Component definitions
# The FBThrift_XXX_FOUND variables below define package components that
# can be checked by callers using "find_package(FBThrift COMPONENTS XXX)"
# e.g., "find_package(FBThrift COMPONENTS py)"
#

# We always build the cpp2 generator and its libraries
set(FBThrift_cpp2_FOUND True)

# Set FBThrift_py_FOUND if we built the thrift/lib/py library that supports
# the output from the thrift compiler's "py" generator.
# (Note that this generator is deprecated in favor of the "py3" generator,)
set(FBThrift_py_FOUND TRUE)

set(FBThrift_FOUND True)
check_required_components(FBThrift)

if (FBThrift_FOUND AND NOT FBThrift_FIND_QUIETLY)
    message(STATUS "Found FBThrift: ${FBTHRIFT_PREFIX_DIR}")
endif()

