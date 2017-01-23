project( vidtk_klt )

# This is version 1.3.4 of the Birchfield KLT implementation.
set(vidtk_klt_VERSION "1.3.4")

set( source_files
                         base.h
  error.c                error.h
  klt_util.c             klt_util.h
  convolve.c             convolve.h
  klt.c                  klt.h
  #pnmio.c                pnmio.h
  pyramid.c              pyramid.h
  selectGoodFeatures.c
  storeFeatures.c
  trackFeatures.c
  #writeFeatures.c
)

add_library( vidtk_klt ${source_files} )
if( UNIX )
  target_link_libraries( vidtk_klt -lm )
endif()
if(VIDTK_CONFIG_ENABLE_SSE2)
  set_source_files_properties( convolve.c convolve.h klt_util.c klt_util.h
                               PROPERTIES COMPILE_FLAGS "-DVIDTK_SSE2=1")
endif()
set_target_properties( vidtk_klt PROPERTIES
  VERSION ${vidtk_klt_VERSION} SOVERSION ${vidtk_klt_VERSION}
)
install( TARGETS vidtk_klt EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
export( TARGETS vidtk_klt APPEND FILE "${VIDTK_EXPORT_FILE}")

if( VIDTK_ACTIVITY_SDK_INSTALL )
  install( TARGETS vidtk_klt
    EXPORT activity-sdk-targets DESTINATION activity_sdk/lib )
endif()
if( VIDTK_EVENT_SDK_INSTALL )
  install( TARGETS vidtk_klt
    EXPORT event-sdk-targets DESTINATION event_sdk/lib )
endif()

if( VIDTK_COMMON_SDK_INSTALL )
   install( TARGETS vidtk_klt
     EXPORT common-sdk-targets DESTINATION common_sdk/lib )
endif( VIDTK_COMMON_SDK_INSTALL )
