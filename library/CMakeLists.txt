project( vidtk )

find_package( VXL REQUIRED )
include(${VXL_CMAKE_DIR}/UseVXL.cmake)
include(GitVersionTest)
check_reachable_git_hash(VXL ${VXL_CMAKE_DIR} "32ab06055371e91191404e40ad6273c5808b401a")

set( BOOST_ROOT "" CACHE PATH "The root path to Boost" )
find_package( Boost 1.42 REQUIRED COMPONENTS thread )

if( VIDTK_ENABLE_OPENCV )
find_package( OpenCV REQUIRED )
endif( VIDTK_ENABLE_OPENCV )

add_definitions( -DBOOST_ALL_NO_LIB )
include_directories( ${Boost_INCLUDE_DIRS} )

include_directories( ${vidtk_SOURCE_DIR} ${vidtk_BINARY_DIR} )

include_directories( ${VXL_VCL_INCLUDE_DIR} )
include_directories( ${VXL_CORE_INCLUDE_DIR} )

add_subdirectory( klt )
add_subdirectory( kwklt )

add_subdirectory( utilities )
add_subdirectory( logger )
add_subdirectory( process_framework )
add_subdirectory( pipeline )
add_subdirectory( video )
add_subdirectory( tracking )
if( VXL_VGUI_FOUND )
add_subdirectory( tracking_gui )
set( vidtk_tracking_gui_library ";vidtk_tracking_gui" )
endif( VXL_VGUI_FOUND )
add_subdirectory( learning )

set( VIDTK_LIBRARIES
  "vidtk_utilities;vidtk_process_framework;vidtk_pipeline;vidtk_video;vidtk_tracking${vidtk_tracking_gui_library};vidtk_learning"
  CACHE INTERNAL "" )

include( ${CMAKE_SOURCE_DIR}/CMake/doxygen.cmake )

if( DOXYGEN_FOUND )
  set( DOCUMENTATION_OUTPUT_PATH
    ${CMAKE_BINARY_DIR}/doc )
#  create_doxygen(
#    ${DOCUMENTATION_OUTPUT_PATH}
#    ${CMAKE_CURRENT_SOURCE_DIR}/dir
#    module
#    ${DOCUMENTATION_OUTPUT_PATH}/othermodule.tag=../othermodule
#    ${DOCUMENTATION_OUTPUT_PATH}/othermodule2.tag=../othermodule2)
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
endif()
