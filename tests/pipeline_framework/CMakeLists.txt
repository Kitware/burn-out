# Tests that don't require any arguments at runtime
#
set( no_argument_test_sources
  test_connect_duplicate_input.cxx
  test_connect_no_execute_down.cxx
  test_connect_no_execute_up.cxx
  test_dependency_processing.cxx
  test_flush_signal.cxx
  test_nested_pipelines.cxx
  test_nested_skip_pipeline.cxx
  test_nested_skipping_pipeline.cxx
  test_no_output_signal.cxx
  test_multi_push_pipeline.cxx
  test_simple_pipelines.cxx
  test_skip_detection_pipeline.cxx
  test_skip_optional_pipeline.cxx
  test_skip_pipeline.cxx
  test_stop_pipeline.cxx
  test_reset_pipeline.cxx
  test_termination.cxx
)

#include( ${CMAKE_SOURCE_DIR}/CMake/ProcessorCount.cmake )
#
#ProcessorCount( PROCESSOR_COUNT )
#
#if( ${PROCESSOR_COUNT} GREATER 1 )
#  set( no_argument_test_sources
#    ${no_argument_test_sources}
#    test_timing_pipeline.cxx
#  )
#endif()

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

add_executable( vidtk_test_pipeline
   ${test_sources}
)
target_link_libraries( vidtk_test_pipeline vidtk_pipeline_framework testlib vul vpl )

# Create ADD_TEST lines for some tests automatically.  Tests taking
# special arguments will be specified manually later.

foreach( filename ${no_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_pipeline_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_pipeline> ${filebase} )
endforeach( filename )

foreach( filename ${data_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_pipeline_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_pipeline> ${filebase}
                    ${vidtk_tests_SOURCE_DIR}/data )
endforeach( filename )

#if( ${PROCESSOR_COUNT} GREATER 1 )
#  set_tests_properties( vidtk_pipeline_timing_pipeline
#    PROPERTIES
#      RUN_SERIAL ON )
#endif()

set_tests_properties( vidtk_pipeline_connect_duplicate_input
  PROPERTIES
    WILL_FAIL TRUE )
set_tests_properties( vidtk_pipeline_connect_no_execute_down
  PROPERTIES
    WILL_FAIL TRUE )
set_tests_properties( vidtk_pipeline_connect_no_execute_up
  PROPERTIES
    WILL_FAIL TRUE )

# This test should run fast and always pass.  The test is for infinite
# loops so set the timeout very small.
set_tests_properties( vidtk_pipeline_termination
  PROPERTIES
    TIMEOUT 5 )
