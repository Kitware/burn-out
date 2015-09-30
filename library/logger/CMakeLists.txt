#ckwg +4
# Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
# KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
# Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.

include (CheckFunctionExists)

set ( LOG_USE_LOG4CXX "OFF" CACHE BOOL "Make Log4cxx available to logger implementation" )
if (LOG_USE_LOG4CXX)

  find_package (Log4cxx REQUIRED)
  # find_package (ApacheRunTime REQUIRED)
  set (USE_LOG4CXX 1)

else(LOG_USE_LOG4CXX)

  unset ( Log4cxx_FOUND       CACHE )
  unset ( Log4cxx_INCLUDE_DIR CACHE )
  unset ( Log4cxx_LIBRARY     CACHE )

  # unset ( ApacheRunTime_FOUND       CACHE )
  # unset ( ApacheRunTime_INCLUDE_DIR CACHE )
  # unset ( ApacheRunTime_LIBRARY     CACHE )

endif(LOG_USE_LOG4CXX)

CHECK_FUNCTION_EXISTS( gethostname   HAVE_GETHOSTNAME )

set( vidtk_logger_sources
     logger.h
     location_info.h                 location_info.cxx
     logger_factory.h                logger_factory.cxx
     logger_factory_mini_logger.h    logger_factory_mini_logger.cxx
     logger_manager.h                logger_manager.cxx
     vidtk_logger.h                  vidtk_logger.cxx
     vidtk_logger_mini_logger.h      vidtk_logger_mini_logger.cxx
     vidtk_mini_logger_formatter.h   vidtk_mini_logger_formatter.cxx
     vidtk_mini_logger_formatter_basic.h   vidtk_mini_logger_formatter_basic.cxx
     vidtk_mini_logger_formatter_legacy.h   vidtk_mini_logger_formatter_legacy.cxx
     vidtk_mini_logger_formatter_json.h    vidtk_mini_logger_formatter_json.cxx
     legacy_logger.cxx
  )

if (LOG_USE_LOG4CXX)
   set ( log4cxx_source
         vidtk_logger_log4cxx.h          vidtk_logger_log4cxx.cxx
         logger_factory_log4cxx.h        logger_factory_log4cxx.cxx
       )

   set ( log4cxx_lib     ${Log4cxx_LIBRARY} ${ApacheRunTime_LIBRARY} )
   include_directories ( ${Log4cxx_INCLUDE_DIR} ${ApacheRunTime_INCLUDE_DIR} )

endif()

include_directories( ${VXL_GEL_INCLUDE_DIR} )

add_library( vidtk_logger ${vidtk_logger_sources} ${log4cxx_source} )

set(feature_definitions)
if( USE_LOG4CXX )
  set(feature_definitions
    ${feature_definitions} "USE_LOG4CXX")
endif()
if( HAVE_GETHOSTNAME )
  set(feature_definitions
    ${feature_definitions} "HAVE_GETHOSTNAME")
endif()

set_target_properties( vidtk_logger PROPERTIES
  VERSION              ${vidtk_VERSION}
  SOVERSION            ${vidtk_VERSION}
  COMPILE_DEFINITIONS  "${feature_definitions}"
)

target_link_libraries( vidtk_logger
                       ${log4cxx_lib}
                       vcl vul vpl )

add_executable( test_logger test_logger.cxx)
target_link_libraries( test_logger vidtk_logger vcl vul
                       vidtk_utilities ${log4cxx_lib} )

install( TARGETS vidtk_logger EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
export( TARGETS vidtk_logger APPEND FILE "${VIDTK_EXPORT_FILE}" )

if( VIDTK_ACTIVITY_SDK_INSTALL )
   install( TARGETS vidtk_logger
     EXPORT activity-sdk-targets DESTINATION activity_sdk/lib )
endif()

if( VIDTK_EVENT_SDK_INSTALL)
   install( TARGETS vidtk_logger
     EXPORT event-sdk-targets DESTINATION event_sdk/lib )
endif()

if( VIDTK_COMMON_SDK_INSTALL )
   install( TARGETS vidtk_logger
     EXPORT common-sdk-targets DESTINATION common_sdk/lib )
endif( VIDTK_COMMON_SDK_INSTALL )
