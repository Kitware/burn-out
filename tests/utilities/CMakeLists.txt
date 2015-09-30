# Tests that don't require any arguments at runtime
#
set( no_argument_test_sources
  test_property_map.cxx
  test_timestamp.cxx
  test_ring_buffer_process.cxx
  test_greedy_assignment.cxx
  test_transform_homography_process.cxx
  test_filter_video_metadata_process.cxx
  test_format_block.cxx
  test_reader_writer.cxx
  test_uuid.cxx
  test_token_expansion.cxx
  test_homography_scoring.cxx
  test_paired_buffer_process.cxx
)

# Tests that take the data directory as the only argument at runtime
#
set( data_argument_test_sources
  test_image_histogram.cxx
  test_config_block.cxx
)

create_test_sourcelist( test_sources
  test_driver.cxx

  # List of source files
  ${no_argument_test_sources}
  ${data_argument_test_sources}
)

add_executable( vidtk_test_utilities
   ${test_sources}
)
target_link_libraries( vidtk_test_utilities vidtk_utilities testlib vul vpl )

# Create tests for each source test file that doesn't take an
# argument.  Tests that do take arguments are manually specified later.

foreach( filename ${no_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_utilities_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_utilities> ${filebase} )
endforeach( filename )

foreach( filename ${data_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_utilities_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_utilities> ${filebase}
                    ${vidtk_tests_SOURCE_DIR}/data )
endforeach( filename )
