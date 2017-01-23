find_package(Threads REQUIRED)

include( CheckFunctionExists )
check_function_exists( setproctitle HAVE_SETPROCTITLE )
set( feature_definitions )
if( HAVE_SETPROCTITLE )
  set( feature_definitions
    ${feature_definitions}
    "HAVE_SETPROCTITLE" )
endif()

set (feature_libs )
set (include_dirs )

OPTION ( USE_RIGHT_TRACK "Use Right Track event tracking library." false )
if ( USE_RIGHT_TRACK )
   find_library (RIGHT_TRACK_LIB right_track )
   set (feature_libs ${RIGHT_TRACK_LIB} vcl vpl vul vsl )
   set (feature_definitions "USE_RIGHT_TRACK")
   SET ( RIGHT_TRACK_DIR "" CACHE FILEPATH "Location of include files" )
   set (include_dirs ${RIGHT_TRACK_DIR} )
   include_directories ( ${include_dirs} )
endif()

set( vidtk_pipeline_framework_SOURCES
  pipeline.h                     pipeline.cxx
  pipeline_node.h                pipeline_node.cxx
  pipeline_edge.h                pipeline_edge.cxx
  pipeline_queue_monitor.h       pipeline_queue_monitor.cxx
  sync_pipeline.h                sync_pipeline.cxx
  sync_pipeline_node.h           sync_pipeline_node.cxx
  sync_pipeline_edge.h
  async_pipeline.h               async_pipeline.cxx
  async_pipeline_node.h          async_pipeline_node.cxx
  async_pipeline_edge.h
  super_process.h                super_process.cxx
)
add_library( vidtk_pipeline_framework ${vidtk_pipeline_framework_SOURCES} )
set_target_properties( vidtk_pipeline_framework PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
  COMPILE_DEFINITIONS "${feature_definitions}"
)

target_link_libraries( vidtk_pipeline_framework
                       ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}
                       vidtk_process_framework vidtk_utilities_no_process vul vcl
                       ${feature_libs} )
install( TARGETS vidtk_pipeline_framework EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
export( TARGETS vidtk_pipeline_framework APPEND FILE "${VIDTK_EXPORT_FILE}")

if( VIDTK_ACTIVITY_SDK_INSTALL )
   install( TARGETS vidtk_pipeline_framework
     EXPORT activity-sdk-targets DESTINATION activity_sdk/lib )
endif( VIDTK_ACTIVITY_SDK_INSTALL )

if( VIDTK_EVENT_SDK_INSTALL )
   install( TARGETS vidtk_pipeline_framework
     EXPORT event-sdk-targets DESTINATION event_sdk/lib )
endif( VIDTK_EVENT_SDK_INSTALL )

if( VIDTK_COMMON_SDK_INSTALL )
   install( TARGETS vidtk_pipeline_framework
     EXPORT common-sdk-targets DESTINATION common_sdk/lib )
endif( VIDTK_COMMON_SDK_INSTALL )
