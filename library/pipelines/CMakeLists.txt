set( vidtk_pipelines_sources
  preprocessing_super_process.h       preprocessing_super_process.txx
  remove_burnin_pipeline.h            remove_burnin_pipeline.txx
  )

aux_source_directory( Templates vidtk_pipelines_sources )

add_library( vidtk_pipelines ${vidtk_pipelines_sources} )

target_link_libraries( vidtk_pipelines
  vidtk_utilities vidtk_process_framework vidtk_pipeline_framework vidtk_tracking
  vidtk_classifiers vidtk_video_io vidtk_video_transforms vidtk_video_properties
  vidtk_object_detectors vidtk_descriptor )

install( TARGETS vidtk_pipelines EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
export( TARGETS vidtk_pipelines APPEND FILE "${VIDTK_EXPORT_FILE}")

if( VIDTK_ACTIVITY_SDK_INSTALL )
  install( TARGETS vidtk_pipelines
    EXPORT activity-sdk-targets DESTINATION activity_sdk/lib )
endif()
if( VIDTK_EVENT_SDK_INSTALL )
  install( TARGETS vidtk_pipelines
    EXPORT event-sdk-targets DESTINATION event_sdk/lib )
endif()
if( VIDTK_COMMON_SDK_INSTALL )
  install( TARGETS vidtk_pipelines
    EXPORT common-sdk-targets DESTINATION common_sdk/lib )
endif()
