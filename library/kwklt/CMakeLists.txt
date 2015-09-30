set( vidtk_kwklt_sources
  klt_pyramid_process.h              klt_pyramid_process.cxx
  klt_track.h                        klt_track.cxx
  klt_tracking_process.h             klt_tracking_process.cxx
  klt_tracking_process_impl.h        klt_tracking_process_impl.cxx
  klt_tracking_process_impl_klt.h    klt_tracking_process_impl_klt.cxx
  klt_util.h                         klt_util.cxx
  klt_mutex.h                        klt_mutex.cxx
  vil_pyramid_image_view_instances.cxx
)

add_library( vidtk_kwklt ${vidtk_kwklt_sources} )
set_target_properties( vidtk_kwklt PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
)
target_link_libraries( vidtk_kwklt vidtk_klt vil )
install( TARGETS vidtk_kwklt EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
export( TARGETS vidtk_kwklt APPEND FILE "${VIDTK_EXPORT_FILE}")


if( VIDTK_ACTIVITY_SDK_INSTALL )
  install( TARGETS vidtk_kwklt
    EXPORT activity-sdk-targets DESTINATION activity_sdk/lib )
endif( VIDTK_ACTIVITY_SDK_INSTALL )

if( VIDTK_EVENT_SDK_INSTALL )
  install( TARGETS vidtk_kwklt
    EXPORT event-sdk-targets DESTINATION event_sdk/lib )
endif( VIDTK_EVENT_SDK_INSTALL )

if( VIDTK_COMMON_SDK_INSTALL )
 install( TARGETS vidtk_kwklt
     EXPORT common-sdk-targets DESTINATION common_sdk/lib )
endif( VIDTK_COMMON_SDK_INSTALL )
