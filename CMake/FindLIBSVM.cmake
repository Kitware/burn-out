#ckwg +4
# Copyright 2012 2014 by Kitware, Inc. All Rights Reserved. Please refer to
# KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
# Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.

# Locate the system installed Libsvm
# The following variables will be set:
#
# LIBSVM_FOUND       - Set to true if Libsvm can be found
# LIBSVM_INCLUDE_DIR - The path to the Libsvm header files
# LIBSVM_LIBRARY     - The full path to the Libsvm library

if( LIBSVM_DIR )
  find_package( LIBSVM NO_MODULE )
elseif( NOT LIBSVM_FOUND )
  include(CommonFindMacros)

  setup_find_root_context(LIBSVM)
  find_path( LIBSVM_INCLUDE_DIR svm.h ${LIBSVM_FIND_OPTS})
  find_library( LIBSVM_LIBRARY svm ${LIBSVM_FIND_OPTS})
  restore_find_root_context(LIBSVM)

  include( FindPackageHandleStandardArgs )
  FIND_PACKAGE_HANDLE_STANDARD_ARGS( LIBSVM DEFAULT_MSG LIBSVM_INCLUDE_DIR LIBSVM_LIBRARY )
  MARK_AS_ADVANCED( LIBSVM_INCLUDE_DIR VLIBSVM_LIBRARY )

endif()
