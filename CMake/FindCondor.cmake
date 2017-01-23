#ckwg +4
# Copyright 2012 2014 by Kitware, Inc. All Rights Reserved. Please refer to
# KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
# Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.

# Locate the system installed Condor API
#
# The following variables will guide the build:
#
# Condor_ROOT        - Set to the install prefix of the Condor library
#
# The following variables will be set:
#
# CONDOR_FOUND       - Set to true if Condor can be found
# CONDOR_INCLUDE_DIR - The path to the Condor header files
# CONDOR_LIBRARY     - The full path to the Condor library

if( NOT CONDOR_FOUND )
  include(CommonFindMacros)

  setup_find_root_context(Condor)
  find_path( CONDOR_INCLUDE_DIR CondorBasic.h ${Condor_FIND_OPTS})
  find_library( CONDOR_LIBRARY condor ${Condor_FIND_OPTS})
  restore_find_root_context(Condor)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args( CONDOR CONDOR_INCLUDE_DIR CONDOR_LIBRARY )
endif()
