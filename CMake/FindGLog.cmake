#ckwg +4
# Copyright 2018 by Kitware, Inc. All Rights Reserved. Please refer to
# KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
# Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.

# Locate the system installed GLog
# The following variables will be set:
#
# GLog_FOUND       - Set to true if GLog can be found
# GLog_INCLUDE_DIR - The path to the GLog header files
# GLog_LIBRARY     - The full path to the GLog library

if( GLog_DIR )
  find_package( GLog NO_MODULE )
elseif( NOT GLog_FOUND )
  include( CommonFindMacros )

  setup_find_root_context( GLog )
  find_path( GLog_INCLUDE_DIR glog/logging.h ${GLog_FIND_OPTS} )
  find_library( GLog_LIBRARY glog ${GLog_FIND_OPTS} )
  restore_find_root_context( GLog )

  include( FindPackageHandleStandardArgs )
  find_package_handle_standard_args( GLog GLog_INCLUDE_DIR GLog_LIBRARY )
  mark_as_advanced( GLog_INCLUDE_DIR GLog_LIBRARY )

endif()
