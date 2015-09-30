include_directories(${KWSYS_INCLUDE_DIR})

set( vidtk_process_framework_SOURCES
  pipeline_aid.h
  process.h                      process.cxx
  external_process_interface.h   external_process_interface_impl.h
  external_proxy_process.h       external_proxy_process.cxx
  external_base_process.h        external_base_process.cxx
  vbl_smart_ptr_instances.cxx
)

add_library( vidtk_process_framework ${vidtk_process_framework_SOURCES} )
set_target_properties( vidtk_process_framework PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
)
target_link_libraries( vidtk_process_framework
  vidtksys
  vidtk_utilities_no_process 
  vcl 
)
install( TARGETS vidtk_process_framework EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
install( FILES 
  function_caller_2_process.txx
  DESTINATION include/process_framework
)
export( TARGETS vidtk_process_framework APPEND FILE "${VIDTK_EXPORT_FILE}")

if( VIDTK_ACTIVITY_SDK_INSTALL )
   install( TARGETS vidtk_process_framework
     EXPORT activity-sdk-targets DESTINATION activity_sdk/lib )
endif( VIDTK_ACTIVITY_SDK_INSTALL )

if( VIDTK_EVENT_SDK_INSTALL )
   install( TARGETS vidtk_process_framework
     EXPORT event-sdk-targets DESTINATION event_sdk/lib )
endif( VIDTK_EVENT_SDK_INSTALL )

if( VIDTK_COMMON_SDK_INSTALL )
   install( TARGETS vidtk_process_framework
     EXPORT common-sdk-targets DESTINATION common_sdk/lib )
endif( VIDTK_COMMON_SDK_INSTALL )
