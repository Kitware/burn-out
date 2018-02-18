set( vidtk_descriptor_sources
  bag_of_words_model.h             bag_of_words_model.txx
  bag_of_words_image_descriptor.h  bag_of_words_image_descriptor.txx
  descriptor_filter.h              descriptor_filter.cxx
  multi_descriptor_generator.h                  multi_descriptor_generator.cxx
  online_descriptor_buffer.h                    online_descriptor_buffer.cxx
  online_descriptor_helper_functions.h          online_descriptor_helper_functions.cxx
  online_descriptor_generator.h                 online_descriptor_generator.cxx
  online_descriptor_computer_process.h          online_descriptor_computer_process.txx
  online_descriptor_thread_system.h             online_descriptor_thread_system.cxx
  online_track_type_classifier.h                online_track_type_classifier.cxx
  meds_generator.h                 meds_generator.cxx
  net_descriptor_computer.h        net_descriptor_computer.cxx
  integral_hog_descriptor.h        integral_hog_descriptor.txx
  integral_hog_descriptor.cxx
  integral_hog_generator.h         integral_hog_generator.txx
  icosahedron_hog_descriptor.h     icosahedron_hog_descriptor.cxx
  icosahedron_hog_generator.h      icosahedron_hog_generator.txx
  tot_adaboost_classifier.h        tot_adaboost_classifier.cxx
  tot_collector_prob_product.h     tot_collector_prob_product.cxx
  tot_collector_process.h          tot_collector_process.cxx
  tot_descriptor_generator.h       tot_descriptor_generator.cxx
  tot_super_process.h              tot_super_process.txx
)

aux_source_directory(Templates vidtk_descriptor_sources)

if( VIDTK_ENABLE_OPENCV )
  set( vidtk_descriptor_sources
    ${vidtk_descriptor_sources}
    color_descriptor_helpers_opencv.h    color_descriptor_helpers_opencv.cxx
    dpm_tot_generator.h                  dpm_tot_generator.cxx
    multi_color_descriptor_generator.h  multi_color_descriptor_generator.txx
    simple_color_histogram_generator.h   simple_color_histogram_generator.txx
    pvohog_generator.h                   pvohog_generator.cxx
    pvohog_track_data.h                  pvohog_track_data.cxx
    sthog_descriptor.h                   sthog_descriptor.cxx
    sthog_cache.h
    )

  if( WIN32 )
    set( vidtk_descriptor_sources
      ${vidtk_descriptor_sources}
      sthog_cache.cxx
      )
  endif()
endif()


if( VIDTK_ENABLE_VLFEAT )
  set( vidtk_descriptor_sources
    ${vidtk_descriptor_sources}
    dsift_bow_descriptor.h   dsift_bow_descriptor.cxx
    dsift_bow_generator.h    dsift_bow_generator.txx
    mfeat_bow_descriptor.h   mfeat_bow_descriptor.cxx
    mfeat_bow_generator.h    mfeat_bow_generator.txx
    )

  if( WIN32 )
    add_definitions(-DNOMINMAX)
  endif()

endif()

if( VIDTK_ENABLE_CAFFE )
  set( vidtk_descriptor_sources
    ${vidtk_descriptor_sources}
    cnn_descriptor.h         cnn_descriptor.cxx
    cnn_generator.h          cnn_generator.txx
  )
endif()

add_library( vidtk_descriptor ${vidtk_descriptor_sources} )
set_target_properties( vidtk_descriptor PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
)

if( VIDTK_ENABLE_OPENCV )
  target_link_libraries( vidtk_descriptor vidtk_learning ${OpenCV_LIBS} vidtk_utilities vgl_algo )
endif()

if( VIDTK_ENABLE_VLFEAT )
  target_link_libraries( vidtk_descriptor ${VLFeat_LIBRARY} )
endif()

if( VIDTK_ENABLE_CAFFE )
  target_link_libraries( vidtk_descriptor ${Caffe_LIBRARIES} ${GLog_LIBRARY} )
endif()

target_link_libraries( vidtk_descriptor vcl vbl vnl vgl vnl_algo vil_algo vidl
                       vidtk_tracking_data vidtk_tracking_data_io vidtk_video_transforms
                       vidtk_learning vidtk_utilities_no_process )

install( TARGETS vidtk_descriptor EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
export( TARGETS vidtk_descriptor APPEND FILE "${VIDTK_EXPORT_FILE}")

if( VIDTK_ACTIVITY_SDK_INSTALL )
  install( TARGETS vidtk_descriptor
    EXPORT activity-sdk-targets DESTINATION activity_sdk/lib )
endif( VIDTK_ACTIVITY_SDK_INSTALL )

if( VIDTK_EVENT_SDK_INSTALL )
  install( TARGETS vidtk_descriptor
    EXPORT event-sdk-targets DESTINATION event_sdk/lib )
endif( VIDTK_EVENT_SDK_INSTALL )

if( VIDTK_COMMON_SDK_INSTALL )
  install( TARGETS vidtk_descriptor
    EXPORT common-sdk-targets DESTINATION common_sdk/lib )
endif( VIDTK_COMMON_SDK_INSTALL )
