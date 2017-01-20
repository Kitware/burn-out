#ckwg +4
# Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
# KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
# Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.

include (CheckFunctionExists)
include (GenerateExportHeader)

find_package(Threads REQUIRED)

set ( LOG_USE_LOG4CXX "OFF" CACHE BOOL "Make Log4cxx available to logger implementation" )
if (LOG_USE_LOG4CXX)

  find_package (Log4cxx REQUIRED)
  # find_package (ApacheRunTime REQUIRED)
  set (USE_LOG4CXX 1)

else()

  unset ( Log4cxx_DIR         CACHE )
  unset ( Log4cxx_FOUND       CACHE )
  unset ( Log4cxx_INCLUDE_DIR CACHE )
  unset ( Log4cxx_LIBRARY     CACHE )

  # unset ( ApacheRunTime_FOUND       CACHE )
  # unset ( ApacheRunTime_INCLUDE_DIR CACHE )
  # unset ( ApacheRunTime_LIBRARY     CACHE )

endif()

set( ARCH_LIBS )
if ("${CMAKE_SYSTEM}" MATCHES "Linux")

  set( ARCH_LIBS rt )  # needed for linux

endif()

CHECK_FUNCTION_EXISTS( gethostname   HAVE_GETHOSTNAME )

include_directories(SYSTEM ${KWSYS_INCLUDE_DIR})

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

     class_loader.h                    class_loader.cxx
  )

if (VIDTK_ENABLE_LIBJSON)
  set(logger_json_src vidtk_mini_logger_formatter_json.h vidtk_mini_logger_formatter_json.cxx)
  set(json_lib ${LIBJSON_LIBRARY})
endif()

###
#              build library to support loadable loggers
# This library supplies all the logger specific code that a plugin needs.
# This is important in cases where a loadable back end is created
# for a project and the logger source code is not available.
add_library( vidtk_logger_plugin_base
    logger_factory.h                      logger_factory.cxx
    vidtk_logger.h                        vidtk_logger.cxx
    location_info.h                       location_info.cxx
    )

target_link_libraries( vidtk_logger_plugin_base
                      ${Boost_THREAD_LIBRARY}
                      ${Boost_SYSTEM_LIBRARY}
                      ${Boost_DATE_TIME_LIBRARY}
                      ${CMAKE_THREAD_LIBS_INIT}
                      ${ARCH_LIBS}
                      vul
                      )

 set_target_properties( vidtk_logger_plugin_base PROPERTIES
      VERSION              ${vidtk_VERSION}
      SOVERSION            ${vidtk_VERSION}
      )


###
# collect logger backends options for installation
set( logger_backends         vidtk_logger_plugin_base )

###
# skip all loadable backends if on windows
if (NOT WIN32)

  ###
  # build log4cxx loadable backend
  if (LOG_USE_LOG4CXX)
    set ( log4cxx_source
      vidtk_logger_log4cxx.h          vidtk_logger_log4cxx.cxx
      logger_factory_log4cxx.h        logger_factory_log4cxx.cxx
      )

    set ( log4cxx_lib     ${Log4cxx_LIBRARY} ${ApacheRunTime_LIBRARY} )
    include_directories ( SYSTEM ${Log4cxx_INCLUDE_DIR} ${ApacheRunTime_INCLUDE_DIR} )

    # build logger backend plugins
    vidtk_add_plugin(NAME vidtk_logger_log4cxx
      SOURCES ${log4cxx_source}
      LINK_LIBRARIES ${log4cxx_lib} vidtk_logger_plugin_base
      )

    list( APPEND logger_backends  vidtk_logger_log4cxx )
    generate_export_header( vidtk_logger_log4cxx )

  endif(LOG_USE_LOG4CXX)
endif( NOT WIN32 )

###
# build mini-logger loadable back end
#
# This is a mess, but it is only done as a proof of concept. Any rational
# implementation would leave the mini-logger as linked into the main logger
# library and not try to make it loadable, (at least not without refactoring
# other software to reduce the accidental dependencies).

vidtk_add_plugin(NAME vidtk_logger_mini_logger
  SOURCES
  logger_factory_mini_logger.h          logger_factory_mini_logger.cxx
  vidtk_logger_mini_logger.h            vidtk_logger_mini_logger.cxx
  vidtk_mini_logger_formatter.h         vidtk_mini_logger_formatter.cxx
  vidtk_mini_logger_formatter_basic.h   vidtk_mini_logger_formatter_basic.cxx
  LINK_LIBRARIES     vidtk_logger_plugin_base vpl
  )

list( APPEND logger_backends      vidtk_logger_mini_logger )
generate_export_header( vidtk_logger_mini_logger )

###
# Build main logger library
add_library( vidtk_logger
             ${vidtk_logger_sources}
             ${logger_json_src})

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
                       vul vpl
                       vidtksys
                       ${Boost_SYSTEM_LIBRARY}
                       ${Boost_DATE_TIME_LIBRARY}
                       ${json_lib}
                       ${Boost_THREAD_LIBRARY}
                       ${CMAKE_THREAD_LIBS_INIT}
                       ${ARCH_LIBS}
 )

add_executable( test_logger test_logger.cxx)
target_link_libraries( test_logger
                       ${Boost_SYSTEM_LIBRARY}
                       vidtk_logger )

install( TARGETS vidtk_logger ${logger_backends} EXPORT vidtk
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib )

install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
export( TARGETS vidtk_logger APPEND FILE "${VIDTK_EXPORT_FILE}" )

if( VIDTK_ACTIVITY_SDK_INSTALL )
   install( TARGETS vidtk_logger ${logger_backends}
     EXPORT activity-sdk-targets DESTINATION activity_sdk/lib )
endif()

if( VIDTK_EVENT_SDK_INSTALL)
   install( TARGETS vidtk_logger ${logger_backends}
     EXPORT event-sdk-targets DESTINATION event_sdk/lib )
endif()

if( VIDTK_COMMON_SDK_INSTALL )
   install( TARGETS vidtk_logger ${logger_backends}
     EXPORT common-sdk-targets DESTINATION common_sdk/lib )
endif( VIDTK_COMMON_SDK_INSTALL )
