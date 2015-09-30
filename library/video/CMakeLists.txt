find_package(Boost REQUIRED
  COMPONENTS
    filesystem
    system)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

include_directories( ${VXL_BRL_INCLUDE_DIR} )

set( vidtk_video_sources
  crop_image_process.h                    crop_image_process.txx
  crop_image_process_instances.cxx
  clamp_image_pixel_values_process.h      clamp_image_pixel_values_process.txx
  clamp_image_pixel_values_process_instances.cxx
  convolve_1d.h
  deep_copy_image_process.h               deep_copy_image_process.txx
  deep_copy_image_process_instances.cxx
  draw_data.h                             draw_data.txx
  draw_data_instances.cxx
  draw_on_image_process.h                 draw_on_image_process.txx
  draw_on_image_process_instances.cxx
  draw_text.h                             draw_text.txx
  draw_text_instances.cxx
  dual_rof_denoise.cxx                    dual_rof_denoise.h
  eo_ir_detector.h                        eo_ir_detector.txx
  eo_ir_detector_instances.cxx
  frame_process.h                         frame_process.txx
  gauss_filter.h
  jittered_image_difference.h             jittered_image_difference.txx
  jittered_image_difference_instances.cxx
  gaussian_pyramid_builder.h              gaussian_pyramid_builder.txx
  generic_frame_process.h                 generic_frame_process.txx
  generic_frame_process.cxx               generic_frame_process_instances.cxx
  greyscale_process.h                     greyscale_process.txx
  greyscale_process_instances.cxx
  identify_metadata_burnin_process.h      identify_metadata_burnin_process.cxx
  image_list_frame_process.h              image_list_frame_process.txx
  image_list_frame_process_instances.cxx
  image_list_frame_metadata_process.h     image_list_frame_metadata_process.txx
  image_list_frame_metadata_process_instances.cxx
  image_list_writer_process.h             image_list_writer_process.txx
  image_list_writer_process_instances.cxx
  image_sequence_accessor.h               image_sequence_accessor.cxx
  mask_image_process.h                    mask_image_process.cxx
  mask_merge_process.h                    mask_merge_process.cxx
  mask_select_process.h                   mask_select_process.cxx
  mask_overlay_process.h                  mask_overlay_process.cxx
  metadata_burnin_filter_process.h        metadata_burnin_filter_process.cxx
  metadata_mask_super_process.h           metadata_mask_super_process.txx
  metadata_mask_super_process_instances.cxx
  threshold_image_process.h               threshold_image_process.txx
  threshold_image_process_instances.cxx
  uncrop_image_process.h                  uncrop_image_process.txx
  uncrop_image_process_instances.cxx
  vidl_ffmpeg_frame_process.h             vidl_ffmpeg_frame_process.cxx
  vidl_ffmpeg_writer_process.h            vidl_ffmpeg_writer_process.cxx
  visualize_world_super_process.h         visualize_world_super_process.txx
  visualize_world_super_process_instances.cxx
  warp_image.h                            warp_image.txx
  warp_image.cxx                          warp_image_instances.cxx
  warp_image_process.h                    warp_image_process.txx
  warp_image_process_instances.cxx
  windowed_mean_image_process.h           windowed_mean_image_process.txx
  windowed_mean_image_process_instances.cxx
  zscore_image.h                          zscore_image.txx
  zscore_image_instances.cxx
)

AUX_SOURCE_DIRECTORY(Templates vidtk_video_sources)

if( VIDTK_ENABLE_OPENCV AND OpenCV_FOUND )
set( vidtk_video_sources
  ${vidtk_video_sources}
  identify_metadata_burnin_process_impl_opencv.h identify_metadata_burnin_process_impl_opencv.cxx
  identify_metadata_burnin_process_static.h identify_metadata_burnin_process_static.cxx
  metadata_inpainting_process.h           metadata_inpainting_process.cxx
)
add_definitions(-DUSE_OPENCV)
endif()


add_library( vidtk_video ${vidtk_video_sources} )
set_target_properties( vidtk_video PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
)
if( VIDTK_ENABLE_OPENCV AND OpenCV_FOUND )
  target_link_libraries( vidtk_video ${OpenCV_LIBS} )
endif()
target_link_libraries( vidtk_video
                       vidtk_utilities vidtk_process_framework vidtk_pipeline
                       vidl vgl vil_algo vil vcl ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} )

install( TARGETS vidtk_video EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
export( TARGETS vidtk_video APPEND FILE "${VIDTK_EXPORT_FILE}")
