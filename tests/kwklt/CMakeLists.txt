#ckwg +5
# Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
# KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
# Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
#

set( no_argument_test_sources
     test_klt_tracking.cxx
)


set( data_argument_test_sources
)

create_test_sourcelist( test_sources
  test_driver.cxx

  # List of source files
  ${no_argument_test_sources}
  ${data_argument_test_sources}
)

add_executable( vidtk_test_kwklt
   ${test_sources}
)
target_link_libraries( vidtk_test_kwklt vidtk_utilities testlib
   vidtk_pipeline vidtk_kwklt )

# Create tests for each source test file that doesn't take an
# argument.  Tests that do take arguments are manually specified later.

foreach( filename ${no_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_kwklt_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_kwklt> ${filebase} )
endforeach( filename )

foreach( filename ${data_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_kwklt_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_kwklt> ${filebase}
                    ${vidtk_tests_SOURCE_DIR}/data )
endforeach( filename )
