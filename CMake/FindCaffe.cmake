#ckwg +4
# Copyright 2016 by Kitware, Inc. All Rights Reserved. Please refer to
# KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
# Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.

# Locate the system installed Caffe
# The following variables will be set:
#
# Caffe_FOUND       - Set to true if Caffe can be found
# Caffe_INCLUDE_DIR - The path to the Caffe header files
# Caffe_LIBRARY     - The full path to the Caffe library

if( Caffe_DIR )
  find_package( Caffe NO_MODULE )
elseif( NOT Caffe_FOUND )
  include(CommonFindMacros)

  setup_find_root_context(Caffe)
  find_path( Caffe_INCLUDE_DIR caffe/caffe.hpp ${Caffe_FIND_OPTS})
  find_library( Caffe_LIBRARY caffe ${Caffe_FIND_OPTS})
  restore_find_root_context(Caffe)

  include( FindPackageHandleStandardArgs )
  find_package_handle_standard_args( Caffe Caffe_INCLUDE_DIR Caffe_LIBRARY )
  mark_as_advanced( Caffe_INCLUDE_DIR Caffe_LIBRARY )

  set( Caffe_INCLUDE_DIRS ${Caffe_INCLUDE_DIR} )
  set( Caffe_LIBRARIES ${Caffe_LIBRARY} )

endif()
