# Tests that don't require any arguments at runtime
#
set( no_argument_test_sources
)

# Tests that take the data directory as the only argument at runtime
#
set( data_argument_test_sources
  test_border_detection_process.cxx
)

create_test_sourcelist( test_sources
  test_driver.cxx

  # List of source files
  ${no_argument_test_sources}
  ${data_argument_test_sources}
)

add_executable( vidtk_test_video_properties
   ${test_sources}
)
target_link_libraries( vidtk_test_video_properties vidtk_video_properties vidtk_video_io vgl_algo testlib vnl vil )

# Create ADD_TEST lines for some tests automatically.  Tests taking
# special arguments will be specified manually later.

foreach( filename ${no_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_video_properties_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_video_properties> ${filebase} )
endforeach( filename )

foreach( filename ${data_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_video_properties_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_video_properties> ${filebase}
                    ${vidtk_tests_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR} )
endforeach( filename )
