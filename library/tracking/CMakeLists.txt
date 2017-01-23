set( vidtk_tracking_sources
  frame_to_frame_homography_process.h           frame_to_frame_homography_process.cxx
  homography_process.h                          homography_process.cxx
  homography_super_process.h                    homography_super_process.cxx
  refine_homography.h                           refine_homography.cxx
  shot_detection_process.h                      shot_detection_process.cxx
  shot_break_flags_process.h                    shot_break_flags_process.cxx
  shot_stitching_algo.h
  generic_shot_stitching_algo.h                 generic_shot_stitching_algo.txx
  generic_shot_stitching_algo.cxx
  klt_shot_stitching_algo.h                     klt_shot_stitching_algo.txx
  shot_stitching_process.h                      shot_stitching_process.txx
  stabilization_super_process.h                 stabilization_super_process.txx
  stab_pass_thru_process.h                      stab_pass_thru_process.txx
  stab_homography_source.h                      stab_homography_source.cxx
)

AUX_SOURCE_DIRECTORY(Templates vidtk_tracking_sources)

if( MSVC )
  set_source_files_properties(Templates/detect_and_track_super_process_instances.cxx
      PROPERTIES COMPILE_FLAGS "/bigobj")
  set_source_files_properties(Templates/full_tracking_super_process_instances.cxx
      PROPERTIES COMPILE_FLAGS "/bigobj")

endif()

if( COMPILER_SUPPORTS_CXX11 )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif( COMPILER_SUPPORTS_CXX0X )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

set(vidtk_public_tracking_libs
  vidtk_video_transforms vidtk_utilities vidtk_tracking_data_io
  vidtk_video_io vidtk_process_framework vidtk_pipeline_framework
  vidtk_video_properties vidtk_object_detectors vidtk_resource_pool
  vidtk_geographic vidtk_klt
  rrel rsdl vil_algo vil_io vnl_io vgl_io vsl
  vil vnl vgl_algo vgl vul vpl vidtk_kwklt
  ${Boost_LIBRARIES}
  )

# Add VidCL components if enabled
if( VIDTK_ENABLE_VISCL AND viscl_FOUND )
  list(APPEND vidtk_tracking_sources
    gpu_descr_tracking_process.h               gpu_descr_tracking_process.cxx
  )
  add_definitions(-DUSE_VISCL)
  set( vidtk_private_tracking_libs ${vidtk_private_tracking_libs} viscl_tasks viscl_vxl )
endif()

# Add MAP-Tk components if enabled
if( VIDTK_ENABLE_MAPTK )
  list(APPEND vidtk_tracking_sources
    maptk_shot_stitching_algo.h maptk_shot_stitching_algo.txx
    maptk_tracking_process.h    maptk_tracking_process.cxx
    )
  aux_source_directory(Templates/maptk vidtk_tracking_sources)
  set( vidtk_private_tracking_libs
    ${vidtk_private_tracking_libs}
    ${MAPTK_LIBRARIES} ${VITAL_LIBRARIES}
    )
  #We need to add MAP-Tk to link directories so we can find them at build time.
  link_directories("${MAPTK_DIR}/lib")
endif()

add_library( vidtk_tracking ${vidtk_tracking_sources} )
set_target_properties( vidtk_tracking PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
  )
target_link_libraries( vidtk_tracking LINK_PUBLIC
  ${vidtk_public_tracking_libs}
 )
target_link_libraries( vidtk_tracking LINK_PRIVATE
  ${vidtk_private_tracking_libs}
  )

install( TARGETS vidtk_tracking EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
  )
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
  )
export( TARGETS vidtk_tracking APPEND FILE "${VIDTK_EXPORT_FILE}")

if( VIDTK_ACTIVITY_SDK_INSTALL )
  install( TARGETS vidtk_tracking
    EXPORT activity-sdk-targets DESTINATION activity_sdk/lib )
endif( VIDTK_ACTIVITY_SDK_INSTALL )

if( VIDTK_EVENT_SDK_INSTALL )
  install( TARGETS vidtk_tracking
    EXPORT event-sdk-targets DESTINATION event_sdk/lib )
endif( VIDTK_EVENT_SDK_INSTALL )

if( VIDTK_COMMON_SDK_INSTALL )
 install( TARGETS vidtk_tracking
     EXPORT common-sdk-targets DESTINATION common_sdk/lib )
endif( VIDTK_COMMON_SDK_INSTALL )
