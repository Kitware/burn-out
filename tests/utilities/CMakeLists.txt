# Tests that don't require any arguments at runtime
#
set( no_argument_test_sources
  test_aoi.cxx
  test_property_map.cxx
  test_timestamp.cxx
  test_ring_buffer_process.cxx
  test_greedy_assignment.cxx
  test_transform_homography_process.cxx
  test_metadata_f2f_homography_process.cxx
  test_project_vidtk_homography_process.cxx
  test_filter_video_metadata_process.cxx
  test_format_block.cxx
  test_reader_writer.cxx
  test_uuid.cxx
  test_token_expansion.cxx
  test_homography_holder_process.cxx
  test_homography_scoring.cxx
  test_horizon_detection_functions.cxx
  test_interpolate_corners_from_shift.cxx
  test_frame_downsampler.cxx
  test_frame_rate_estimator.cxx
  test_homography.cxx
  test_paired_buffer_process.cxx
  test_stream_filter.cxx
  test_large_file_fstream_with_large_file.cxx
  test_geo_coord.cxx
  test_split_string.cxx
  test_compute_transformation.cxx
  test_tag_reader_writer_process.cxx
  test_video_modality.cxx
  test_tcp_string_reader_process.cxx
  test_queue_process.cxx
  test_string_to_vector.cxx
  test_gsd_file_source.cxx
  test_videoname_prefix.cxx
  test_geo_bounds.cxx
)

# Tests that take the data directory as the only argument at runtime
#
set( data_argument_test_sources
  test_image_histogram.cxx
  test_config_block.cxx
  test_homography_reader_writer_process.cxx
  test_large_file_fstream.cxx
  test_kwa_index.cxx
  test_file_string_reader_process.cxx
  test_video_metadata_writing.cxx
  test_kwa_writer_process.cxx
  test_timestamp_generation.cxx
  test_external_settings.cxx
  test_string_reader_process.cxx
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
target_link_libraries( vidtk_test_utilities vidtk_utilities vidtk_pipeline_framework testlib
  vidl vgl vil_algo vil_io vil vpl )

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
