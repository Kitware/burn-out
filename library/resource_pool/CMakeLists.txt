
set( resource_pool_src
  resource_pool.h                  resource_pool.cxx
  resource_pool_exception.h
  resource_user.h                  resource_user.cxx
  )

add_library( vidtk_resource_pool ${resource_pool_src} )

target_link_libraries( vidtk_resource_pool
  ${Boost_SIGNALS_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  vidtk_logger
  )

install( TARGETS vidtk_resource_pool EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
  )
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h" )

export( TARGETS vidtk_resource_pool APPEND FILE "${VIDTK_EXPORT_FILE}" )

if( VIDTK_ACTIVITY_SDK_INSTALL )
   install( TARGETS vidtk_resource_pool
     EXPORT activity-sdk-targets DESTINATION activity_sdk/lib )
endif()

if( VIDTK_EVENT_SDK_INSTALL)
   install( TARGETS vidtk_resource_pool
     EXPORT event-sdk-targets DESTINATION event_sdk/lib )
endif()

if( VIDTK_COMMON_SDK_INSTALL )
   install( TARGETS vidtk_resource_pool
     EXPORT common-sdk-targets DESTINATION common_sdk/lib )
endif( VIDTK_COMMON_SDK_INSTALL )
