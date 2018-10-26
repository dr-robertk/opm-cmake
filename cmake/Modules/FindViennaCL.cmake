# - Try to find ViennaCL
# Once done this will define
#
#  VIENNACL_FOUND        - system has ViennaCL
#  VIENNACL_INCLUDES     - the ViennaCL include directories
#  OPenCL Variables in addition
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

find_package(OpenCL)

# search VIENNACL header
find_path(VIENNACL_INCLUDE_DIR version.hpp
  PATHS ${VIENNACL_ROOT} ${VIENNACL_DIR}
  PATH_SUFFIXES include include/viennacl
  NO_DEFAULT_PATH
  DOC "Include directory of VIENNACL")

if( VIENNACL_INCLUDE_DIR )
  set(VIENNACL_INCLUDE_DIRS ${VIENNACL_INCLUDE_DIR})

  set(VIENNACL_FOUND TRUE)
endif()

if (VIENNACL_FOUND)
  #set HAVE_VIENNACL for config.h
  set(HAVE_VIENNACL 1)
endif()

if(OpenCL_FOUND)
  #set HAVE_OPENCL for config.h
  set(HAVE_OPENCL 1)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ViennaCL DEFAULT_MSG VIENNACL_INCLUDE_DIRS)
mark_as_advanced(VIENNACL_INCLUDE_DIRS)
find_package_handle_standard_args(OpenCL DEFAULT_MSG OpenCL_INCLUDE_DIRS OpenCL_LIBRARIES)
mark_as_advanced(OpenCL_INCLUDE_DIRS OpenCL_LIBRARIES)
