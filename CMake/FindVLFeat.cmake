#ckwg +4
# Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
# KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
# Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.

# Locate the system installed VLFeat
# The following variables will be set:
#
# VLFeat_FOUND       - Set to true if VLFeat can be found
# VLFeat_INCLUDE_DIR - The path to the VLFeat header files
# VLFeat_LIBRARY     - The full path to the VLFeat library

if( VLFeat_DIR )
  find_package( VLFeat NO_MODULE )
elseif( NOT VLFEAT_FOUND )
  include(CommonFindMacros)

  setup_find_root_context(VLFeat)
  find_path( VLFeat_INCLUDE_DIR vl/sift.h ${VLFeat_FIND_OPTS})
  find_library( VLFeat_LIBRARY vl ${VLFeat_FIND_OPTS})
  restore_find_root_context(VLFeat)

  include( FindPackageHandleStandardArgs )
  find_package_handle_standard_args( VLFeat VLFeat_INCLUDE_DIR VLFeat_LIBRARY )
  mark_as_advanced( VLFeat_INCLUDE_DIR VLFeat_LIBRARY )

endif()
