project( vidtk )

include( vidtk_target_utils )

if( VIDTK_ENABLE_LIBSVM )
  find_package( LIBSVM REQUIRED )
  include_directories(SYSTEM ${LIBSVM_INCLUDE_DIR})
endif( VIDTK_ENABLE_LIBSVM )

if( VIDTK_ENABLE_TESSERACT )
  find_package( Tesseract REQUIRED )
endif( VIDTK_ENABLE_TESSERACT )

include_directories( ${vidtk_SOURCE_DIR} ${vidtk_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( SYSTEM ${VXL_RPL_INCLUDE_DIR} )
include_directories( SYSTEM ${VXL_VCL_INCLUDE_DIR} )
include_directories( SYSTEM ${VXL_CORE_INCLUDE_DIR} )

add_subdirectory( klt )
add_subdirectory( kwklt )

add_subdirectory( klv )
add_subdirectory( utilities )
add_subdirectory( logger )
add_subdirectory( resource_pool )
add_subdirectory( plugin_loader )

add_subdirectory( process_framework )
add_subdirectory( pipeline_framework )
add_subdirectory( video_io )
add_subdirectory( video_transforms )
add_subdirectory( video_properties )
add_subdirectory( descriptors )
add_subdirectory( learning )
add_subdirectory( classifier )
add_subdirectory( object_detectors )
add_subdirectory( tracking_data )
add_subdirectory( tracking )

add_subdirectory( pipelines )

set( VIDTK_LIBRARIES
  "vidtk_utilities;vidtk_process_framework;vidtk_pipeline_framework;vidtk_video_transforms;vidtk_video_io;vidtk_video_properties;vidtk_tracking_data;vidtk_tracking;vidtk_klv;vidtk_object_detectors;vidtk_descriptor;vidtk_learning;vidtk_classifiers;vidtk_pipelines"
  CACHE INTERNAL "" )

include( ${CMAKE_SOURCE_DIR}/CMake/doxygen.cmake )

if( DOXYGEN_FOUND )
  set( DOCUMENTATION_OUTPUT_PATH
    ${CMAKE_BINARY_DIR}/doc )

  create_doxygen(
    ${DOCUMENTATION_OUTPUT_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}
    VidTK)

  create_doxygen(
    ${DOCUMENTATION_OUTPUT_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    common)

  create_doxygen(
    ${DOCUMENTATION_OUTPUT_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/activity_detectors
    activity_detectors)

  create_doxygen(
    ${DOCUMENTATION_OUTPUT_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/event_detectors
    event_detectors)

  create_doxygen(
    ${DOCUMENTATION_OUTPUT_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/stanag_4676
    stanag_4676)

endif()
