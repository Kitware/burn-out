
# Enable /bigobj flag for Visual Studio 2008 and greater
# Without it, this library won't link (under certain circumstances)
if(MSVC_VERSION GREATER 1400)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /bigobj")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /bigobj")
endif()

include_directories( SYSTEM ${VXL_RPL_INCLUDE_DIR} )
include_directories( SYSTEM ${VXL_BRL_INCLUDE_DIR} )

set( vidtk_object_detector_sources
  blob_pixel_feature_extraction.h           blob_pixel_feature_extraction.txx
  blob_pixel_feature_extraction_process.h   blob_pixel_feature_extraction_process.txx
  conn_comp_super_process.h                 conn_comp_super_process.txx
  conn_comp_pass_thru_process.h             conn_comp_pass_thru_process.txx
  connected_component_process.h             connected_component_process.cxx
  detector_factory.h                        detector_factory.cxx
                                            detector_factory.txx
  detector_implementation_base.h            detector_implementation_base.txx
  detector_implementation_factory.h
  detection_reader_pipeline.h               detection_reader_pipeline.txx
  detector_super_process.h                  detector_super_process.txx
  detector_super_process_port_def.h
  detection_consolidator_process.h          detection_consolidator_process.cxx
  diff_buffer_process.h                     diff_buffer_process.txx
  diff_pass_thru_process.h                  diff_pass_thru_process.txx
  diff_pixel_feature_infusion_process.h     diff_pixel_feature_infusion_process.txx
  diff_super_process.h                      diff_super_process.txx
  filter_image_objects_process.h            filter_image_objects_process.txx
  fg_image_process.h
  ghost_detector.h                          ghost_detector.txx
  groundcam_salient_region_classifier.h     groundcam_salient_region_classifier.txx
  homography_to_scale_image_process.h       homography_to_scale_image_process.txx
  image_difference_no_bkgnd_process.h       image_difference_no_bkgnd_process.txx
  moving_training_data_container.h          moving_training_data_container.txx
  pixel_annotation_loader.h                 pixel_annotation_loader.cxx
  pixel_feature_extractor_super_process.h   pixel_feature_extractor_super_process.txx
  pixel_feature_writer.h                    pixel_feature_writer.txx
  project_to_world_process.h                project_to_world_process.cxx
  maritime_salient_region_classifier.h      maritime_salient_region_classifier.txx
  metadata_mask_super_process.h             metadata_mask_super_process.txx
  metadata_text_parser_process.h            metadata_text_parser_process.txx
  model_selector.h
  motion_detector_pipeline.h                motion_detector_pipeline.txx
  motion_and_saliency_detector_pipeline.h   motion_and_saliency_detector_pipeline.txx
  moving_burnin_detector_process.h          moving_burnin_detector_process.cxx
  obj_specific_salient_region_classifier.h  obj_specific_salient_region_classifier.txx
  obj_specific_salient_region_detector.h    obj_specific_salient_region_detector.txx
  osd_mask_refiner.h                        osd_mask_refiner.txx
  osd_mask_refinement_process.h             osd_mask_refinement_process.txx
  osd_recognizer.h                          osd_recognizer.txx
  osd_recognizer_process.h                  osd_recognizer_process.txx
  osd_template.h                            osd_template.cxx
  saliency_detector_super_process.h         saliency_detector_super_process.txx
  salient_region_classifier.h
  salient_region_classifier_process.h       salient_region_classifier_process.txx
  scene_obstruction_detector.h              scene_obstruction_detector.txx
  scene_obstruction_detector_process.h      scene_obstruction_detector_process.txx
  simple_burnin_filter_process.h            simple_burnin_filter_process.txx
  sg_background_model_process.h             sg_background_model_process.txx
  sg_gm_pixel_model.h                       sg_gm_pixel_model.cxx
  text_parser.h                             text_parser.txx
  threaded_detector.h
  three_frame_differencing.h                three_frame_differencing.txx
  three_frame_differencing_process.h        three_frame_differencing_process.txx
  track_reinit_detector.h                   track_reinit_detector.cxx
  transform_image_object_functors.h         transform_image_object_functors.txx
  transform_image_object_process.h          transform_image_object_process.txx
)

if( VIDTK_ENABLE_OPENCV )
  set( vidtk_object_detector_sources
    ${vidtk_object_detector_sources}
    moving_burnin_detector_opencv.h           moving_burnin_detector_opencv.cxx
    ocv_hog_detector.h                        ocv_hog_detector.cxx
    ocv_hog_detector_pipeline.h               ocv_hog_detector_pipeline.txx
    ocv_hog_detector_process.h                ocv_hog_detector_process.cxx
    ocv_multi_image_hog_process.h             ocv_multi_image_hog_process.cxx
    ocv_training_display.h                    ocv_training_display.cxx
    sthog_detector.h                          sthog_detector.cxx
    sthog_detector_process.h                  sthog_detector_process.txx
    )
endif()

if( VIDTK_ENABLE_CAFFE )
  set( vidtk_object_detector_sources
    ${vidtk_object_detector_sources}
    cnn_detector.h                            cnn_detector.txx
    cnn_detector_process.h                    cnn_detector_process.txx
    cnn_detector_pipeline.h                   cnn_detector_pipeline.txx
    motion_and_cnn_detector_pipeline.h        motion_and_cnn_detector_pipeline.txx
  )
endif()

if( VIDTK_ENABLE_TESSERACT )
  add_definitions(-DUSE_TESSERACT)
endif()

if( VXL_BRL_FOUND )
  include_directories( SYSTEM ${VXL_BRL_INCLUDE_DIR}/bseg )
  list(APPEND vidtk_object_detector_sources
    gmm_background_model_process.h          gmm_background_model_process.txx)
  add_definitions(-DUSE_BRL)
else()
  message(STATUS "BRL not built with VXL, skipping gmm")
endif()

AUX_SOURCE_DIRECTORY( Templates vidtk_object_detector_sources )

add_library( vidtk_object_detectors ${vidtk_object_detector_sources} )

target_link_libraries( vidtk_object_detectors
  vidtk_utilities vidtk_process_framework vidtk_pipeline_framework
  vidtk_vil_plugins vidtk_resource_pool vidtk_classifiers
  vidtk_video_io vidtk_descriptor vidtk_video_transforms
  vidtk_video_properties vidtk_tracking_data_io
  vidl vgl vgl_algo vil_algo vil_io vil
  ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY}
)

set_target_properties( vidtk_object_detectors PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
  )

if( VIDTK_ENABLE_TESSERACT )
  include_directories( SYSTEM ${Tesseract_INCLUDE_DIR} )
  target_link_libraries( vidtk_object_detectors ${Tesseract_LIBRARY} )
endif()

install( TARGETS vidtk_object_detectors EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
export( TARGETS vidtk_object_detectors APPEND FILE "${VIDTK_EXPORT_FILE}")

if( VIDTK_ACTIVITY_SDK_INSTALL )
  install( TARGETS vidtk_object_detectors
    EXPORT activity-sdk-targets DESTINATION activity_sdk/lib )
endif()
if( VIDTK_EVENT_SDK_INSTALL )
  install( TARGETS vidtk_object_detectors
    EXPORT event-sdk-targets DESTINATION event_sdk/lib )
endif()
if( VIDTK_COMMON_SDK_INSTALL )
  install( TARGETS vidtk_object_detectors
    EXPORT common-sdk-targets DESTINATION common_sdk/lib )
endif()
