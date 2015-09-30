#ckwg +5
# Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
# KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
# Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
#

find_package ( LIBJSON REQUIRED )
include_directories ( ${LIBJSON_INCLUDE_DIR} )

set( no_argument_test_sources
     test_json_formatter.cxx
     test_location_info.cxx
     test_logger_manager.cxx
     test_logger.cxx
)


set( data_argument_test_sources
)

create_test_sourcelist( test_sources
  test_driver.cxx

  # List of source files
  ${no_argument_test_sources}
  ${data_argument_test_sources}
)

add_executable( vidtk_test_logger
   ${test_sources}
)
target_link_libraries( vidtk_test_logger vidtk_utilities testlib
  vul vpl vidtk_logger ${LIBJSON_LIBRARY} )

# Create tests for each source test file that doesn't take an
# argument.  Tests that do take arguments are manually specified later.

foreach( filename ${no_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_logger_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_logger> ${filebase} )
endforeach( filename )

foreach( filename ${data_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_logger_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_logger> ${filebase}
                    ${vidtk_tests_SOURCE_DIR}/data )
endforeach( filename )
