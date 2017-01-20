#ckwg +4
# Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
# KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
# Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.

# Locate the system installed Tesseract
# The following variables will be set:
#
# Tesseract_FOUND       - Set to true if Tesseract can be found
# Tesseract_INCLUDE_DIR - The path to the Tesseract header files
# Tesseract_LIBRARY     - The full path to the Tesseract library

if( Tesseract_DIR )
  find_package( Tesseract NO_MODULE )
elseif( NOT Tesseract_FOUND )
  include(CommonFindMacros)

  setup_find_root_context(Tesseract)
  find_path( Tesseract_INCLUDE_DIR tesseract/baseapi.h ${Tesseract_FIND_OPTS})
  find_library( Tesseract_LIBRARY tesseract ${Tesseract_FIND_OPTS})
  restore_find_root_context(Tesseract)

  include( FindPackageHandleStandardArgs )
  find_package_handle_standard_args( Tesseract Tesseract_INCLUDE_DIR Tesseract_LIBRARY )
  mark_as_advanced( Tesseract_INCLUDE_DIR Tesseract_LIBRARY )

endif()
