# Tests that don't require any arguments at runtime
#
set( no_argument_test_sources
  test_deep_copy_image_process.cxx
  test_mask_image_process.cxx
  test_warp_image.cxx
  test_crop_image_process.cxx
  test_uncrop_image_process.cxx
  test_jittered_image_difference.cxx
  test_threshold_image_process.cxx
  test_zscore.cxx
)

# Tests that take the data directory as the only argument at runtime
#
set( data_argument_test_sources
  test_image_list_frame_process.cxx
  test_vidl_ffmpeg_frame_process.cxx
  test_dual_rof_denoise.cxx
  test_gaussian_pyramid_builder.cxx
)

create_test_sourcelist( test_sources
  test_driver.cxx

  # List of source files
  ${no_argument_test_sources}
  ${data_argument_test_sources}
)

configure_file( ${vidtk_tests_SOURCE_DIR}/data/smallframe_file.txt
                ${CMAKE_CURRENT_BINARY_DIR}/smallframe_file.txt )

add_executable( vidtk_test_video
   ${test_sources}
)
target_link_libraries( vidtk_test_video vidtk_video vgl_algo testlib vnl vul vpl )

# Create ADD_TEST lines for some tests automatically.  Tests taking
# special arguments will be specified manually later.

foreach( filename ${no_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_video_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_video> ${filebase} )
endforeach( filename )

foreach( filename ${data_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_video_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_video> ${filebase}
                    ${vidtk_tests_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR} )
endforeach( filename )
