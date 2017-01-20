
set( vidtk_video_io_sources
  frame_process.h                         frame_process.txx
  frame_process_sp_facade_interface.h     frame_process_sp_facade.h
  frame_metadata_decoder_process.h        frame_metadata_decoder_process.txx
  filename_frame_metadata_process.h       filename_frame_metadata_process.txx
  generic_frame_process.h                 generic_frame_process.txx
  generic_frame_process.cxx
  image_list_frame_process.h              image_list_frame_process.txx
  image_list_frame_metadata_process.h     image_list_frame_metadata_process.txx
  image_list_writer_process.h             image_list_writer_process.txx
  image_sequence_accessor.h               image_sequence_accessor.cxx
  frame_metadata_super_process.h          frame_metadata_super_process.txx
  vidl_ffmpeg_frame_process.h             vidl_ffmpeg_frame_process.cxx
  vidl_ffmpeg_metadata_frame_process.h    vidl_ffmpeg_metadata_frame_process.cxx
  vidl_ffmpeg_writer_process.h            vidl_ffmpeg_writer_process.cxx
  qt_ffmpeg_writer_process.h              qt_ffmpeg_writer_process.cxx
  mask_reader_process.h                   mask_reader_process.cxx
)

if(VIDTK_HAS_GDAL)
  include_directories(SYSTEM ${GDAL_INCLUDE_DIR})
  set(vidtk_video_io_sources ${vidtk_video_io_sources} gdal_nitf_writer.h gdal_nitf_writer.txx)
  add_definitions(-DUSE_GDAL)
endif()

AUX_SOURCE_DIRECTORY(Templates vidtk_video_io_sources)

if(USE_ANGEL_FIRE)
  set( vidtk_video_io_sources
    ${vidtk_video_io_sources}
    )
  set_source_files_properties(
    frame_metadata_super_process.h      frame_metadata_super_process.txx
    Templates/frame_metadata_super_process_instances.cxx
    PROPERTIES COMPILE_FLAGS "-DUSE_ANGEL_FIRE")
endif()

add_library( vidtk_video_io ${vidtk_video_io_sources} )

set( video_io_public_links
  vidtk_utilities vidtk_video_transforms vidtk_process_framework vidtk_pipeline_framework vidtk_vil_plugins
  vidl vgl vil_algo vil_io vil vidtk_klv ${QT_LIBRARIES}
  ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY}
  )
set( video_io_private_links )

if(USE_CONDOR)
  list( APPEND video_io_public_links ${CONDOR_LIBRARY})
endif()

if(USE_ANGEL_FIRE)
  list( APPEND video_io_public_links ${AFREADER_LIBRARIES})
endif()

#ORDER BETWEEN GDAL AND CONDOR MATTERS.  Please do not move the gdal linking before condor linking.
if(VIDTK_HAS_GDAL)
  list( APPEND video_io_public_links ${GDAL_LIBRARY})
endif()

target_link_libraries( vidtk_video_io
  PUBLIC ${video_io_public_links}
  PRIVATE ${video_io_private_links}
)
set_target_properties( vidtk_video_io PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
  )

install( TARGETS vidtk_video_io EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
export( TARGETS vidtk_video_io APPEND FILE "${VIDTK_EXPORT_FILE}")

if( VIDTK_ACTIVITY_SDK_INSTALL )
  install( TARGETS vidtk_video_io
    EXPORT activity-sdk-targets DESTINATION activity_sdk/lib )
endif()

if( VIDTK_EVENT_SDK_INSTALL )
  install( TARGETS vidtk_video_io
    EXPORT event-sdk-targets DESTINATION event_sdk/lib )
endif()

if( VIDTK_COMMON_SDK_INSTALL )
  install( TARGETS vidtk_video_io
    EXPORT common-sdk-targets DESTINATION common_sdk/lib )
endif()

if( VIDTK_ACTIVITY_SDK_INSTALL OR VIDTK_EVENT_SDK_INSTALL )
  install( TARGETS vidtk_video_io EXPORT perseas-sdk-targets DESTINATION perseas/lib )
  install( FILES image_sequence_accessor.h DESTINATION perseas/include/process_framework )
endif()
