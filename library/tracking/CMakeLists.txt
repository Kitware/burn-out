find_package( TinyXML 2.6 REQUIRED )
include_directories( ${TinyXML_INCLUDE_DIR} )

find_package(SHAPELIB REQUIRED)
include_directories(${SHAPELIB_INCLUDE_DIR})

include_directories( ${VXL_RPL_INCLUDE_DIR} )
include_directories( ${VXL_GEL_INCLUDE_DIR}/mrc )

# Required VXL for bundle_adjust_tracks
check_reachable_git_hash(VXL ${VXL_CMAKE_DIR} "7bf4ba6672606e03d4739cee3241979b826fd274")


set( vidtk_tracking_sources
  transform_image_object_functors.h             transform_image_object_functors.txx
                                                transform_image_object_functors_instances.cxx
  transform_image_object_process.h              transform_image_object_process.txx
                                                transform_image_object_process_instances.cxx
  amhi.h                                        amhi.txx
  amhi_create_image_process.h                   amhi_create_image_process.txx
  amhi_data.h                                   amhi_data.cxx
  amhi_create_image_process_instances.cxx
  amhi_instances.cxx
  amhi_initializer_process.h                    amhi_initializer_process.txx
  amhi_initializer_process_instances.cxx
  back_tracking_super_process.h                 back_tracking_super_process.txx
  back_tracking_super_process_instances.cxx
  bundle_adjust_tracks.h                        bundle_adjust_tracks.cxx
  circular_path.h
  compute_color_histogram_process.h             compute_color_histogram_process.txx
  compute_color_histogram_process_instances.cxx
  compute_object_features_process.h             compute_object_features_process.txx
  compute_object_features_process_instances.cxx
  conn_comp_super_process.h                     conn_comp_super_process.txx
  conn_comp_super_process_instances.cxx
  connected_component_process.h                 connected_component_process.cxx
  da_so_tracker_generator.h                     da_so_tracker_generator.cxx
  da_so_tracker_generator_process.h             da_so_tracker_generator_process.cxx
  da_so_tracker_kalman.h                        da_so_tracker_kalman.cxx
  da_so_tracker_kalman_extended.h               da_so_tracker_kalman_extended.txx
  da_so_tracker_kalman_extended_instances.cxx
  da_so_tracker.h                               da_so_tracker.cxx
  da_tracker_process.h                          da_tracker_process.cxx
  debug_tracker.h                               debug_tracker.cxx
  detect_and_track_super_process.h              detect_and_track_super_process.txx
  detect_and_track_super_process_instances.cxx
  diff_buffer_process.h                         diff_buffer_process.txx
  diff_buffer_process_instances.cxx
  diff_pass_thru_process.h                      diff_pass_thru_process.txx
  diff_pass_thru_process_instances.cxx
  diff_super_process.h                          diff_super_process.txx
  diff_super_process_instances.cxx
  extended_kalman_functions.h                   extended_kalman_functions.cxx
  fg_image_process.h
  filter_image_objects_process.h                filter_image_objects_process.txx
  filter_image_objects_process_instances.cxx
  filter_image_process.h                        filter_image_process.cxx
  filter_tracks_process.h                       filter_tracks_process.cxx
  filter_tracks_process2.h                      filter_tracks_process2.cxx
  finalize_output_process.h                     finalize_output_process.cxx
  following_track_linking_functor.h             following_track_linking_functor.cxx
  fg_matcher.h                                  fg_matcher.txx
  fg_matcher_ssd.h                              fg_matcher_ssd.txx
  fg_matcher_csurf.h                            fg_matcher_csurf.txx
  frame_to_frame_homography_process.h           frame_to_frame_homography_process.cxx
  full_tracking_super_process.h                 full_tracking_super_process.txx
  full_tracking_super_process_instances.cxx
  frame_objects_buffer_process.h                frame_objects_buffer_process.cxx
  gen_world_units_per_pixel_process.h           gen_world_units_per_pixel_process.txx
  gen_world_units_per_pixel_process_instances.cxx
  ghost_detector.h                              ghost_detector.txx
  ghost_detector_instances.cxx
  greyscale_morphology_process.h                greyscale_morphology_process.cxx
  glob_to_step_reader.h                         glob_to_step_reader.cxx
  gui_process.h                                 gui_process.cxx
  gui_process_impl.h
  homography_process.h                          homography_process.cxx
  image_chip_writer_process.h                   image_chip_writer_process.cxx
  image_difference_process.h                    image_difference_process.txx
  image_difference_process_instances.cxx
  image_difference_process2.h                   image_difference_process2.txx
  image_difference_process2_instances.cxx
  image_difference_no_bkgnd_process.h           image_difference_no_bkgnd_process.txx
  image_difference_no_bkgnd_process_instances.cxx
  image_object.h                                image_object.cxx
  image_object.txx                              image_object_instances.cxx
  image_object_writer_process.h                 image_object_writer_process.cxx
  image_object_reader_process.h                 image_object_reader_process.cxx
  interpolate_track_state_process.h             interpolate_track_state_process.cxx
  interpolate_gps_track_state_process.h         interpolate_gps_track_state_process.cxx
  kinematic_track_linking_functor.h             kinematic_track_linking_functor.cxx
  klt_homography_super_process.h                klt_homography_super_process.cxx
  klt_tracker_process.h                         klt_tracker_process.cxx
  kw18_reader.h                                 kw18_reader.cxx
  mit_layer_annotation_reader.h                 mit_layer_annotation_reader.cxx
  linear_path.h
  morphology_process.h                          morphology_process.cxx
  multi_track_linking_functors_functor.h
  multi_track_linking_functors_functor.cxx
  multiple_features.h                           multiple_features.cxx
  multiple_features_process.h                   multiple_features_process.cxx
  orthorectified_metadata_source_process.h      orthorectified_metadata_source_process.txx
  orthorectified_metadata_source_process_instances.cxx
  overlap_break_track_linking_functor.h         overlap_break_track_linking_functor.cxx
  overlap_track_linking_functor.h               overlap_track_linking_functor.cxx
  project_to_world_process.h                    project_to_world_process.cxx
  pvo_probability.h                             pvo_probability.cxx
  ring_buffer_instances.cxx                     ring_buffer_process_instances.cxx
  reconcile_tracks_process.h                    reconcile_tracks_process.cxx
  reconcile_tracks_algo.h                       reconcile_tracks_algo.cxx
  sg_gm_pixel_model.h                           sg_gm_pixel_model.cxx
  sg_background_model_process.h                 sg_background_model_process.txx
  sg_background_model_process_instances.cxx
  shape_file_writer.h                           shape_file_writer.cxx
  shot_detection_process.h                      shot_detection_process.cxx
  shot_break_flags_process.h                    shot_break_flags_process.cxx
  shot_break_monitor.h                          shot_break_monitor.txx
                                                shot_break_monitor_instances.cxx
  shot_stitching_algo.h                         shot_stitching_algo.txx
  shot_stitching_algo_instances.cxx
  shot_stitching_process.h                      shot_stitching_process.txx
  shot_stitching_process_instances.cxx
  stabilization_super_process.h                 stabilization_super_process.txx
  stabilization_super_process_instances.cxx
  stab_pass_thru_process.h                      stab_pass_thru_process.txx
  stab_pass_thru_process_instances.cxx
  synthetic_path_generator_process.h            synthetic_path_generator_process.txx
  synthetic_path_generator_process_instances.cxx
  track.h                                       track.cxx
  track_initializer_process.h                   track_initializer_process.cxx
  track_linking.h                               track_linking.txx
  track_linking_functor.h                       track_linking_functor.cxx
  track_linking_instances.cxx
  track_linking_process.h                       track_linking_process.cxx
  track_reader.h
  track_reader_process.h                        track_reader_process.cxx
  track_sptr.h
  track_state.h                                 track_state.cxx
  track_velocity_init_functions.h               track_velocity_init_functions.cxx
  track_view.h                                  track_view.cxx
  track_writer_process.h                        track_writer_process.cxx
  tracker_cost_function.h                       tracker_cost_function.cxx
  tracker_cost_func_kin.h                       tracker_cost_func_kin.cxx
  tracker_cost_func_color_size_kin_amhi.h       tracker_cost_func_color_size_kin_amhi.cxx
  tracking_keys.h                               tracking_keys.cxx
  tracking_super_process.h                      tracking_super_process.txx
  tracking_super_process_instances.cxx
  transform_tracks_process.h                    transform_tracks_process.txx
  transform_tracks_process_instances.cxx
  unique_track_id_process.h                     unique_track_id_process.cxx
  viper_reader.h                                viper_reader.cxx
  visual_track_linking_functor.h                visual_track_linking_functor.cxx
  vsl_reader.h                                  vsl_reader.cxx
  vsl/image_object_io.h                         vsl/image_object_io.cxx
  vsl/image_object_old_io.h                     vsl/image_object_old_io.cxx
  vsl/track_io.h                                vsl/track_io.cxx
  vsl/track_old_io.h                            vsl/track_old_io.cxx
  wh_tracker_kalman.h                           wh_tracker_kalman.cxx
  world_connected_component_process.h           world_connected_component_process.cxx
  world_coord_super_process.h                   world_coord_super_process.txx
  world_coord_super_process_instances.cxx
  world_morphology_process.h                    world_morphology_process.cxx
  world_smooth_image_process.h                  world_smooth_image_process.txx
  world_smooth_image_process_instances.cxx
)

AUX_SOURCE_DIRECTORY(Templates vidtk_tracking_sources)

if(BRL_FOUND)
 include_directories( ${VXL_BRL_INCLUDE_DIR} )
 list(APPEND vidtk_tracking_sources
   gmm_background_model_process.h gmm_background_model_process.txx
   gmm_background_model_process_instances.cxx)
else()
  message(STATUS "BRL not built with VXL, skipping gmm")
endif()

add_library( vidtk_tracking ${vidtk_tracking_sources} )
set_target_properties( vidtk_tracking PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
)
target_link_libraries( vidtk_tracking
                       vidtk_video vidtk_utilities vidtk_process_framework
                       vidtk_pipeline vidtk_geographic
                       vidtk_klt rrel rsdl vil_algo vil_io vnl_io vgl_io
                       vsl vil vnl vpgl_algo vgl_algo vgl vul vcl vpl vidtk_kwklt
                       ${TinyXML_LIBRARY} ${SHAPELIB_LIBRARY} )
install( TARGETS vidtk_tracking EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
export( TARGETS vidtk_tracking APPEND FILE "${VIDTK_EXPORT_FILE}")


if( VIDTK_ACTIVITY_SDK_INSTALL )
  install( TARGETS vidtk_tracking
    EXPORT activity-sdk-targets DESTINATION activity_sdk/lib )
endif( VIDTK_ACTIVITY_SDK_INSTALL )

if( VIDTK_EVENT_SDK_INSTALL )
  install( TARGETS vidtk_tracking
    EXPORT event-sdk-targets DESTINATION event_sdk/lib )
endif( VIDTK_EVENT_SDK_INSTALL )

if( VIDTK_COMMON_SDK_INSTALL )
 install( TARGETS vidtk_tracking
     EXPORT common-sdk-targets DESTINATION common_sdk/lib )
endif( VIDTK_COMMON_SDK_INSTALL )
