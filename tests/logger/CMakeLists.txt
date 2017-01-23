#ckwg +5
# Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
# KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
# Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
#

if (VIDTK_ENABLE_LIBJSON)
  set(logger_json_src test_json_formatter.cxx)
  set(json_lib ${LIBJSON_LIBRARY})
endif()

set( no_argument_test_sources
     test_location_info.cxx
     test_logger_manager.cxx
     test_logger.cxx
     ${logger_json_src}
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
   vul vpl vidtk_logger ${json_lib})

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
