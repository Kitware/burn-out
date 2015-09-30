find_package(Boost REQUIRED
  COMPONENTS
    filesystem
    system)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

include_directories( ${VXL_GEL_INCLUDE_DIR} )
set( vidtk_utilities_sources
  unchecked_return_value.h          unchecked_return_value.cxx
  checked_bool.h                    checked_bool.cxx
  config_block.h                    config_block.cxx
                                    config_block.txx
  config_block_parser.h             config_block_parser.cxx
  compute_transformations.h         compute_transformations.cxx
  log.h
  property_map.h                    property_map.cxx
  timestamp.h                       timestamp.cxx
  vsl/timestamp_io.h                vsl/timestamp_io.cxx
  format_block.h                    format_block.cxx
  buffer.h
  ring_buffer.h                     ring_buffer.txx
                                    ring_buffer_instances.cxx
  barcode_factory.h                 barcode_factory.txx
                                    barcode_factory_instances.cxx
  timestamp_image.h                 timestamp_image.txx
                                    timestamp_image_instances.cxx
  oc_timestamp_hack.h               oc_timestamp_hack.cxx
  image_histogram.h                 image_histogram.txx
  image_histogram_instances.cxx
  vsl/image_histogram_io.h          vsl/image_histogram_io.txx
  vsl/image_histogram_io.cxx
  greedy_assignment.h               greedy_assignment.cxx
  utm_coordinate.h                  utm_coordinate.cxx
  convert_latitude_longitude_utm.h  convert_latitude_longitude_utm.cxx
  compute_transformations.h         compute_transformations.cxx
  ffmt.h                            ffmt.cxx
  video_metadata.h                  video_metadata.cxx
  homography.h                      homography.cxx
  compute_gsd.h                     compute_gsd.cxx
  video_modality.h                  video_modality.cxx
  base_reader_writer.h              base_reader_writer.cxx
  group_data_reader_writer.h        group_data_reader_writer.cxx
  timestamp_reader_writer.h
  video_metadata_reader_writer.h    video_metadata_vector_reader_writer.h
  video_modality_reader_writer.h
  gsd_reader_writer.h
  shot_break_flags_reader_writer.h
  image_view_reader_writer.h
  kw_archive_writer.h               kw_archive_writer.cxx
  token_expansion.h                 token_expansion.cxx
  thread_util.h                     thread_util.cxx
  paired_buffer.h                   paired_buffer.txx
  Templates/paired_buffer_instances.cxx
  uuid_able.h uuid_able.cxx
)

set( vidtk_utilities_process_sources
  apply_function_to_vector_process.h apply_function_to_vector_process.txx
  async_observer_process.h          async_observer_process.txx
  deserialize_process.h             deserialize_process.txx
  file_stream_process.h             file_stream_process.cxx
  filter_video_metadata_process.h   filter_video_metadata_process.cxx
  holder_process.h                  holder_process.txx
                                    holder_process_instances.cxx
  homography_reader_process.h       homography_reader_process.cxx
  homography_holder_process.h       homography_holder_process.cxx
  homography_reader_or_generator_process.h      homography_reader_or_generator_process.cxx
  homography_writer_process.h       homography_writer_process.cxx
  homography_scoring_process.h      homography_scoring_process.cxx
  list_writer_process.h             list_writer_process.txx
  merge_2_vectors_process.h         merge_2_vectors_process.txx
  merge_n_vectors_process.h         merge_n_vectors_process.txx
  metadata_reader_process.h         metadata_reader_process.cxx
  ring_buffer_process.h             ring_buffer_process.txx
                                    ring_buffer_process_instances.cxx
  serialize_process.h               serialize_process.txx
  split_vector_process.h            split_vector_process.txx
  timestamp_reader_process.h        timestamp_reader_process.txx
                                    timestamp_reader_process_instances.cxx
  transform_homography_process.h    transform_homography_process.cxx
  transform_vidtk_homography_process.h transform_vidtk_homography_process.txx
                                       transform_vidtk_homography_process_instances.cxx

  base_io_process.h                  base_io_process.cxx
  base_reader_process.h              base_reader_process.cxx
  base_writer_process.h              base_writer_process.cxx
  tagged_data_writer_process.h       tagged_data_writer_process.cxx
  tagged_data_reader_process.h       tagged_data_reader_process.cxx

  queue_process.h                    queue_process.txx
                                     queue_process_instances.cxx
  kw_archive_writer_process.h        kw_archive_writer_process.cxx
  paired_buffer_process.h            paired_buffer_process.txx
  Templates/paired_buffer_process_instances.cxx
)

add_library( vidtk_utilities_no_process ${vidtk_utilities_sources} )
set_target_properties( vidtk_utilities_no_process PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
)
target_link_libraries( vidtk_utilities_no_process
                       vidtk_logger vidtk_geographic vgl_algo vil vil_io vnl vnl_io vsl vcl vul vpgl ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} )
install( TARGETS vidtk_utilities_no_process EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
export( TARGETS vidtk_utilities_no_process APPEND FILE "${VIDTK_EXPORT_FILE}" )

add_library( vidtk_utilities ${vidtk_utilities_process_sources} )
set_target_properties( vidtk_utilities PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
)
target_link_libraries( vidtk_utilities
                       vidtk_utilities_no_process vidtk_process_framework )
install( TARGETS vidtk_utilities EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
export( TARGETS vidtk_utilities APPEND FILE "${VIDTK_EXPORT_FILE}" )

install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
install( FILES
  apply_function_to_vector_process.txx
  config_block.txx
  demux_pairs_process.txx
  deserialize_process.txx
  list_writer_process.txx
  merge_2_vectors_process.txx merge_n_vectors_process.txx
  serialize_process.txx
  split_vector_process.txx
  DESTINATION include/utilities
)

if( VIDTK_ACTIVITY_SDK_INSTALL )
   install( TARGETS vidtk_utilities_no_process
     EXPORT activity-sdk-targets DESTINATION activity_sdk/lib )
   install( TARGETS vidtk_utilities
     EXPORT activity-sdk-targets DESTINATION activity_sdk/lib )
endif()

if( VIDTK_EVENT_SDK_INSTALL)
   install( TARGETS vidtk_utilities_no_process
     EXPORT event-sdk-targets DESTINATION event_sdk/lib )
   install( TARGETS vidtk_utilities
     EXPORT event-sdk-targets DESTINATION event_sdk/lib )
endif()

if( VIDTK_COMMON_SDK_INSTALL )
   install( TARGETS vidtk_utilities_no_process
     EXPORT common-sdk-targets DESTINATION common_sdk/lib )
   install( TARGETS vidtk_utilities
     EXPORT common-sdk-targets DESTINATION common_sdk/lib )
endif( VIDTK_COMMON_SDK_INSTALL )
