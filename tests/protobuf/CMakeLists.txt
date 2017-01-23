# Tests that don't require any arguments at runtime
#
set( no_argument_test_sources
  test_convert_protobuf.cxx
  test_protobuf_container.cxx
  )

set( data_argument_test_sources
  )

create_test_sourcelist( test_sources
  test_driver.cxx

  # List of source files
  ${no_argument_test_sources}
  ${data_argument_test_sources}
  )

add_executable( vidtk_test_protobuf
   ${test_sources}
)
target_link_libraries( vidtk_test_protobuf
  vidtk_protobuf
  vidtk_tracking_data vidtk_utilities
  testlib vnl vul vgl vil )

# Create ADD_TEST lines for some tests automatically.  Tests taking
# special arguments will be specified manually later.

foreach( filename ${no_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_protobuf_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_protobuf> ${filebase} )
endforeach( filename )

foreach( filename ${data_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_protobuf_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_protobuf> ${filebase}
                    ${vidtk_tests_SOURCE_DIR}/data )
endforeach( filename )
