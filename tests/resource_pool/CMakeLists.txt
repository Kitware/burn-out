

set( no_argument_test_sources
  test_resource_user.cxx
  test_resource_pool.cxx
  )

set( data_argument_test_sources
  )

create_test_sourcelist( test_sources
  test_driver.cxx

  # List of source files
  ${no_argument_test_sources}
  ${data_argument_test_sources}
)

add_executable( vidtk_test_resource_pool
   ${test_sources}
)

target_link_libraries( vidtk_test_resource_pool
  testlib
  vidtk_resource_pool
  vidtk_utilities
  vidtk_pipeline_framework
  )

# Create tests for each source test file that doesn't take an
# argument.  Tests that do take arguments are manually specified later.

foreach( filename ${no_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_resource_pool> ${filebase} )
endforeach( filename )

foreach( filename ${data_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_resource_pool>> ${filebase}
                    ${vidtk_tests_SOURCE_DIR}/data )
endforeach( filename )
