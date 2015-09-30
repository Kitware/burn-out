cmake_minimum_required(VERSION 2.6.4)

option(BUILD_SHARED_LIBS "Build with shared libraries."
  ON)

# avoid CMake trying to verify compiler compatibility
set(CMAKE_OVERRIDE_COMPILER_MISMATCH TRUE)
# This will only work with Qt4 so force it
set(DESIRED_QT_VERSION 4 CACHE INTERNAL FORCE "")
set(VTK_DESIRED_QT_VERSION 4)
# Start the project
project(VIRATGeovis)

# find/use VTK
find_package(VTK)
if(NOT VTK_DIR)
  message(FATAL_ERROR "Please set VTK_DIR.")
endif(NOT VTK_DIR)
include(${VTK_USE_FILE})

# use what QVTK built with
if(VTK_QT_QMAKE_EXECUTABLE)
  set(QT_QMAKE_EXECUTABLE ${VTK_QT_QMAKE_EXECUTABLE} CACHE FILEPATH "")
else(VTK_QT_QMAKE_EXECUTABLE)
  message(FATAL_ERROR "VTK Must be built with Qt Enabled")
endif(VTK_QT_QMAKE_EXECUTABLE)
find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})

find_package(VXL)
if(VXL_FOUND)
 include(${VXL_CMAKE_DIR}/UseVXL.cmake)
else(VXL_FOUND)
 message("VXL_DIR should be set to the VXL build directory.")
endif(VXL_FOUND)

# depends on vidtk_video for reading videos
#find_package(vidtk_video)

find_path(VIDTK_VIDEO_INCLUDE_DIR vidl_ffmpeg_frame_process.h
${CMAKE_CURRENT_SOURCE_DIR}/../../library/video
)

find_library(VIDTK_VIDEO_LIBRARY libvidtk_video
${CMAKE_CURRENT_BINARY_DIR}
)

# Use the include path and library for Qt that is used by VTK.
include_directories(
  ${QT_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${VIDTK_VIDEO_INCLUDE_DIR}/..
)

message("VIDTK_VIDEO_INCLUDE_DIR=${VIDTK_VIDEO_INCLUDE_DIR}")

# Set your files and resources here  
set(VIRATGeovisSrcs
    main.cxx
    VIRATGeovis.cxx
    vidMapView.cxx
#    vidRichedVideo.cxx
    vidSceneObserver.cxx
    vidVideoReader.cxx
    vidVideoController.cxx
    vidMonitor.cxx
    DialogQueries.cxx)

set(vidLibSrcs
   vidVideoMetaDataReader.cxx
   vidQueryResultsReader.cxx
   vidStep.cxx
   vidTrack.cxx
   vidTrackVisualProperty.cxx
   vidTrackRepresentation.cxx
   vidBBox.cxx
   vidTrackMetaData.cxx
)

set(VIRATGeovisUI VIRATGeovis.ui DialogQueries.ui)
set(VIRATGeovisHeaders
     VIRATGeovis.h
     DialogQueries.h
     vidVideoController.h)

set(VIRATGeovisResources Icons/icons.qrc)

set(TestDecoderSrcs TestDecoder.cxx)
set(TestDecoderMetaSrcs TestDecoderMeta.cxx)
set(TestTrackReaderSrcs TestTrackReader.cxx)

# The rest should just work (sure...)
qt4_wrap_ui(UISrcs ${VIRATGeovisUI})
qt4_wrap_cpp(MOCSrcs ${VIRATGeovisHeaders} )
qt4_add_resources(ResourceSrcs ${VIRATGeovisResources})

source_group("Resources" FILES
  ${VIRATGeovisUI}
  ${VIRATGeovisResources}
  ${EXE_ICON}
)

source_group("Generated" FILES
  ${UISrcs}
  ${MOCSrcs}
  ${ResourceSrcs}
  ${RCS_SOURCES}
)

add_definitions(-DQT_GUI_LIBS -DQT_CORE_LIB -DQT3_SUPPORT)
set_source_files_properties(${VIRATGeovisSrcs} PROPERTIES
                            OBJECT_DEPENDS "${UISrcs}")

# It's nice to have the ui in the windows project file...just double click on
# it and designer comes up on that ui file :) 
set (VIRATGeovisSrcs ${VIRATGeovisSrcs} ${VIRATGeovisUI})

add_executable( VIRATGeovis MACOSX_BUNDLE ${VIRATGeovisSrcs} 
  ${UISrcs} ${MOCSrcs} ${ResourceSrcs})

add_executable(TestDecoder MACOSX_BUNDLE ${TestDecoderSrcs})
add_executable(TestDecoderMeta MACOSX_BUNDLE ${TestDecoderMetaSrcs})
add_executable(TestTrackReader MACOSX_BUNDLE ${TestTrackReaderSrcs})

add_library(vidLib ${vidLibSrcs})
target_link_libraries(vidLib vtkGeovis)

target_link_libraries( VIRATGeovis
  QVTK
  ${QT_LIBRARIES}
  vtkGeovis
  ${VIDTK_VIDEO_LIBRARY}
  vidLib
)

target_link_libraries(TestDecoder
  ${VIDTK_VIDEO_LIBRARY}
)
target_link_libraries(TestDecoderMeta
  ${VIDTK_VIDEO_LIBRARY}
  vidLib
)
target_link_libraries(TestTrackReader
  ${VIDTK_VIDEO_LIBRARY}
  vidLib
)
