# Tests that don't require any arguments at runtime
#
set( no_argument_test_sources
  test_frame_averaging_process.cxx
  test_frame_downsampling_process.cxx
  test_video_enhancement_process.cxx
  test_nearest_neighbor_inpaint.cxx
  test_integral_image_process.cxx
  test_kmeans_segmentation.cxx
  test_convert_color_space.cxx
  test_deep_copy_image_process.cxx
  test_mask_image_process.cxx
  test_warp_image.cxx
  test_crop_image_process.cxx
  test_uncrop_image_process.cxx
  test_jittered_image_difference.cxx
  test_threshold_image_process.cxx
  test_zscore.cxx
  test_color_commonality_filter_process.cxx
  test_floating_point_image_hash_process.cxx
  test_flow_warp.cxx
  test_threaded_image_transform.cxx
  test_image_derivs.cxx
  test_invert_image_values.cxx
  test_resample.cxx
  test_world_morphology_process.cxx
)

# Tests that take the data directory as the only argument at runtime
#
set( data_argument_test_sources
  test_dual_rof_denoise.cxx
  test_gaussian_pyramid_builder.cxx
  test_super_res.cxx
  test_ssd.cxx
  test_ndi.cxx
)

create_test_sourcelist( test_sources
  test_driver.cxx

  # List of source files
  ${no_argument_test_sources}
  ${data_argument_test_sources}
)

configure_file( ${vidtk_tests_SOURCE_DIR}/data/smallframe_file.txt
                ${CMAKE_CURRENT_BINARY_DIR}/smallframe_file.txt )

add_executable( vidtk_test_video_transforms
   ${test_sources}
)
target_link_libraries( vidtk_test_video_transforms vidtk_video_transforms
                       vidtk_object_detectors vidtk_video_io vgl_algo testlib vnl vul vpl )

# Create ADD_TEST lines for some tests automatically.  Tests taking
# special arguments will be specified manually later.

foreach( filename ${no_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_video_transforms_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_video_transforms> ${filebase} )
endforeach( filename )

foreach( filename ${data_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_video_transforms_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_video_transforms> ${filebase}
                    ${vidtk_tests_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR} )
endforeach( filename )
