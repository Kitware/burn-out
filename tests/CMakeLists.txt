project( vidtk_tests )

include_directories( ${vidtk_SOURCE_DIR} ${vidtk_BINARY_DIR} )

set( MIDAS_REST_URL "http://ecmidas.kitware.com/midas/api/rest" )
set( MIDAS_KEY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data/midas" )
set( MIDAS_DATA_DIR "${CMAKE_CURRENT_BINARY_DIR}/data/midas" )
include( ${CMAKE_SOURCE_DIR}/CMake/MIDAS3.cmake )

if( VXL_FOUND )

  include( ${VXL_CMAKE_DIR}/UseVXL.cmake )

  include_directories( SYSTEM ${VXL_VCL_INCLUDE_DIR}
                       ${VXL_CORE_INCLUDE_DIR} )
  include_directories( SYSTEM ${VXL_BRL_INCLUDE_DIR} )

  add_subdirectory( process_framework )
  add_subdirectory( utilities )
  add_subdirectory( video_io )
  add_subdirectory( video_transforms )
  add_subdirectory( video_properties )
  add_subdirectory( classifier )
  add_subdirectory( tracking_data )
  add_subdirectory( tracking )
  add_subdirectory( resource_pool )
  add_subdirectory( object_detectors )
  add_subdirectory( pipeline_framework )
  add_subdirectory( logger )

  add_subdirectory( learning )
  add_subdirectory( kwklt )
  add_subdirectory( descriptors )

endif( VXL_FOUND )
