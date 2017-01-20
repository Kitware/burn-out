include_directories( SYSTEM ${VXL_BRL_INCLUDE_DIR} )

set( vidtk_video_transforms_sources
  adjoint_dbw.h                           adjoint_dbw.cxx
  adjoint_flow_warp.h
  adjoint_image_derivs.h
  adjoint_image_op.h                      adjoint_image_op.cxx
  adjoint_image_utils.h
  adjoint_resample.h
  aligned_edge_detection.h                aligned_edge_detection.txx
  aligned_edge_detection_process.h        aligned_edge_detection_process.txx
  automatic_white_balancing.h             automatic_white_balancing.txx
  color_commonality_filter.h              color_commonality_filter.txx
  color_commonality_filter_process.h      color_commonality_filter_process.txx
  convert_color_space.h                   convert_color_space.txx
  convert_color_space.cxx
  convert_color_space_process.h           convert_color_space_process.txx
  convert_image_process.h                 convert_image_process.txx
  crop_image_process.h                    crop_image_process.txx
  clamp_image_pixel_values_process.h      clamp_image_pixel_values_process.txx
  convolve_1d.h
  deep_copy_image_process.h               deep_copy_image_process.txx
  draw_data.h                             draw_data.txx
  draw_on_image_process.h                 draw_on_image_process.txx
  draw_text.h                             draw_text.txx
  dual_rof_denoise.cxx                    dual_rof_denoise.h
  frame_averaging.h                       frame_averaging.txx
  frame_averaging_process.h               frame_averaging_process.txx
  frame_downsampling_process.h            frame_downsampling_process.txx
  gauss_filter.h
  jittered_image_difference.h             jittered_image_difference.txx
  floating_point_image_hash_functions.h
  floating_point_image_hash_process.h     floating_point_image_hash_process.txx
  gaussian_pyramid_builder.h              gaussian_pyramid_builder.txx
  greyscale_process.h                     greyscale_process.txx
  high_pass_filter.h                      high_pass_filter.txx
  high_pass_filter_process.h              high_pass_filter_process.txx
  illumination_normalization.h            illumination_normalization.txx
  invert_image_values.h
  image_consolidator_process.h            image_consolidator_process.txx
  integral_image_process.h                integral_image_process.txx
  inpainting_process.h                    inpainting_process.txx
  kmeans_segmentation.h                   kmeans_segmentation.txx
  kmeans_segmentation_process.h           kmeans_segmentation_process.txx
  mask_image_process.h                    mask_image_process.cxx
  mask_merge_process.h                    mask_merge_process.cxx
  mask_select_process.h                   mask_select_process.cxx
  mask_overlay_process.h                  mask_overlay_process.cxx
  moving_mosaic_generator.h               moving_mosaic_generator.txx
  nearest_neighbor_inpaint.h              nearest_neighbor_inpaint.txx
  normalized_cross_correlation.h
  percentile_image.h                      percentile_image.txx
  rescale_image_process.h                 rescale_image_process.txx
  super_res.h                             super_res.cxx
  super_res_utils.h                       super_res_utils.cxx
  threshold_image_process.h               threshold_image_process.txx
  threaded_image_transform.h
  uncrop_image_process.h                  uncrop_image_process.txx
  variable_image_dilate.h                 variable_image_dilate.txx
  variable_image_erode.h                  variable_image_erode.txx
  video_enhancement_process.h             video_enhancement_process.txx
  vil_for_each.h
  warp_and_average.h                      warp_and_average.txx
  warp_image.h                            warp_image.txx
  warp_image.cxx
  warp_image_process.h                    warp_image_process.txx
  world_morphology_process.h              world_morphology_process.txx
  world_smooth_image_process.h            world_smooth_image_process.txx
  zscore_image.h                          zscore_image.txx
)


AUX_SOURCE_DIRECTORY(Templates vidtk_video_transforms_sources)

set(video_transform_public_libs
  vidtk_process_framework
  )
set(video_transform_private_libs
  vidtk_utilities vgl vil vil_algo
  ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY}
  )

if( VXL_MUL_FOUND )
  include_directories( SYSTEM ${VXL_MUL_INCLUDE_DIR} )
  add_definitions( -DUSE_MUL )
  list( APPEND video_transform_private_libs mbl )
endif()

add_library( vidtk_video_transforms ${vidtk_video_transforms_sources} )

if( VIDTK_ENABLE_OPENCV )
  list( APPEND video_transform_private_libs ${OpenCV_LIBS} )
  add_definitions(-DUSE_OPENCV)
endif()

target_link_libraries( vidtk_video_transforms
  PUBLIC ${video_transform_public_libs}
  PRIVATE ${video_transform_private_libs}
  )

install( TARGETS vidtk_video_transforms EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
export( TARGETS vidtk_video_transforms APPEND FILE "${VIDTK_EXPORT_FILE}")

if( VIDTK_ACTIVITY_SDK_INSTALL )
  install( TARGETS vidtk_video_transforms
    EXPORT activity-sdk-targets DESTINATION activity_sdk/lib )
endif()
if( VIDTK_EVENT_SDK_INSTALL )
  install( TARGETS vidtk_video_transforms
    EXPORT event-sdk-targets DESTINATION event_sdk/lib )
endif()
if( VIDTK_COMMON_SDK_INSTALL )
  install( TARGETS vidtk_video_transforms
    EXPORT common-sdk-targets DESTINATION common_sdk/lib )
endif()

if( VIDTK_ACTIVITY_SDK_INSTALL OR VIDTK_EVENT_SDK_INSTALL )
  install( TARGETS vidtk_video_transforms EXPORT perseas-sdk-targets DESTINATION perseas/lib )
  install( FILES image_sequence_accessor.h DESTINATION perseas/include/process_framework )
endif()
