# Tests that don't require any arguments at runtime
#
set( no_argument_test_sources
  test_function_caller_processes.cxx
)

# Tests that take the data directory as the only argument at runtime
#
set( data_argument_test_sources
)

create_test_sourcelist( test_sources
  test_driver.cxx

  # List of source files
  ${no_argument_test_sources}
  ${data_argument_test_sources}
)

add_executable( vidtk_test_process_framework
   ${test_sources}
)
target_link_libraries( vidtk_test_process_framework vidtk_pipeline_framework vidtk_process_framework testlib vul vpl )

# Create ADD_TEST lines for some tests automatically.  Tests taking
# special arguments will be specified manually later.

foreach( filename ${no_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_process_framework_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_process_framework> ${filebase} )
endforeach( filename )

foreach( filename ${data_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_process_framework_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_process_framework> ${filebase}
                    ${vidtk_tests_SOURCE_DIR}/data )
endforeach( filename )
