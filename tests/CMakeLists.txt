project( vidtk_tests )

find_package( VXL REQUIRED )

set( BOOST_ROOT "" CACHE PATH "The root path to Boost" )
find_package( Boost REQUIRED )
add_definitions( -DBOOST_ALL_NO_LIB )
include_directories( ${Boost_INCLUDE_DIRS} )

include_directories( ${vidtk_SOURCE_DIR} ${vidtk_BINARY_DIR} )

if( VIDTK_ENABLE_OPENCV )
find_package( OpenCV REQUIRED )
endif( VIDTK_ENABLE_OPENCV )

set( MIDAS_REST_URL "http://gemenon/midas/api/rest" )
set( MIDAS_KEY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data/midas" )
set( MIDAS_DATA_DIR "${CMAKE_CURRENT_BINARY_DIR}/data/midas" )
include( ${CMAKE_SOURCE_DIR}/CMake/MIDAS.cmake )

if( VXL_FOUND )

  include( ${VXL_CMAKE_DIR}/UseVXL.cmake )
  include(GitVersionTest)
  check_reachable_git_hash(VXL ${VXL_CMAKE_DIR} "69ee84425ac42d02b7c8598f91fa8cd7d485c458" QUIET)

  include_directories( ${VXL_VCL_INCLUDE_DIR}
                       ${VXL_CORE_INCLUDE_DIR} )
  include_directories( ${VXL_BRL_INCLUDE_DIR} )

  add_subdirectory( process_framework )
  add_subdirectory( utilities )
  add_subdirectory( video )
  add_subdirectory( tracking )
  add_subdirectory( pipeline )
  add_subdirectory( logger )
  add_subdirectory( learning )

endif( VXL_FOUND )

option( VIDTK_ENABLE_PERFORMANCE_TESTS "Enable performance tests" OFF )
if( VIDTK_ENABLE_PERFORMANCE_TESTS )
  add_subdirectory( performance )
endif()
