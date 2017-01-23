#ckwg +4
# Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
# KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
# Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.

# Locate the system installed Apix Interlink API
#
# The following variables will guide the build:
#
# ApixInterlink_ROOT        - Set to the install prefix of the Apix Interlink library
#
# The following variables will be set:
#
# ApixInterlink_FOUND       - Set to true if Apix Interlink can be found
# INTERLINK_INCLUDE_DIR - The path to the Apix Interlink header files
# INTERLINK_LIBRARY     - The full path to the Apix Interlink library

if( ApixInterlink_DIR )
  find_package( ApixInterlink NO_MODULE )
elseif( NOT ApixInterlink_FOUND )
  include(CommonFindMacros)

  setup_find_root_context(ApixInterlink)
  find_path( INTERLINK_INCLUDE_DIR APIX_CInterlink.h ${ApixInterlink_FIND_OPTS})
  find_library( INTERLINK_LIBRARY APIX_Interlink ${ApixInterlink_FIND_OPTS})
  restore_find_root_context(ApixInterlink)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args( ApixInterlink INTERLINK_INCLUDE_DIR INTERLINK_LIBRARY )
endif()
