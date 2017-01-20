
set( vidtk_video_properties_sources
  border_detection.h                      border_detection.txx
  border_detection_process.h              border_detection_process.txx
  eo_ir_detector.h                        eo_ir_detector.txx
  extract_image_resolution_process.h      extract_image_resolution_process.txx
  extract_masked_region_properties.h      extract_masked_region_properties.txx
  filter_image_process.h                  filter_image_process.cxx
  windowed_mean_image_process.h           windowed_mean_image_process.txx
  )

AUX_SOURCE_DIRECTORY( Templates vidtk_video_properties_sources )

add_library( vidtk_video_properties ${vidtk_video_properties_sources} )

set( video_properties_public_links
  vidtk_utilities_no_process vidtk_process_framework
  vidtk_tracking_data vil
  )
set( video_properties_private_links vgl )

target_link_libraries( vidtk_video_properties
  PUBLIC ${video_properties_public_links}
  PRIVATE ${video_properties_private_links}
)

set_target_properties( vidtk_video_properties PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
  )

install( TARGETS vidtk_video_properties EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
export( TARGETS vidtk_video_properties APPEND FILE "${VIDTK_EXPORT_FILE}")

if( VIDTK_ACTIVITY_SDK_INSTALL )
  install( TARGETS vidtk_video_properties
    EXPORT activity-sdk-targets DESTINATION activity_sdk/lib )
endif()
if( VIDTK_EVENT_SDK_INSTALL )
  install( TARGETS vidtk_video_properties
    EXPORT event-sdk-targets DESTINATION event_sdk/lib )
endif()
if( VIDTK_COMMON_SDK_INSTALL )
  install( TARGETS vidtk_video_properties
    EXPORT common-sdk-targets DESTINATION common_sdk/lib )
endif()

if( VIDTK_ACTIVITY_SDK_INSTALL OR VIDTK_EVENT_SDK_INSTALL )
  install( TARGETS vidtk_video_properties EXPORT perseas-sdk-targets DESTINATION perseas/lib )
  install( FILES image_sequence_accessor.h DESTINATION perseas/include/process_framework )
endif()
