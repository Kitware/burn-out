find_package(Git)

set(VIDTK_BUILT_FROM_GIT FALSE)

# todo: figure out what this is doing and comment. I don't the see $Format string being generated anywhere
if ("$Format:$" STREQUAL "")
  set(vidtk_git_hash       "$Format:%H$")
  set(vidtk_git_hash_short "$Format:%h$")

  option(VIDTK_IS_PATCHED "Set to ON if patches are applied on top of a released tarball" OFF)
  if (VIDTK_IS_PATCHED)
    set(vidtk_git_dirty "dirty")
  endif (VIDTK_IS_PATCHED)
elseif (GIT_FOUND)
  set(configure_code "
if (IS_DIRECTORY \"${vidtk_all_SOURCE_DIR}/.git\")
  set(VIDTK_BUILT_FROM_GIT TRUE)

  execute_process(
    COMMAND           \"${GIT_EXECUTABLE}\"
                      rev-parse
                      HEAD
    WORKING_DIRECTORY \"${vidtk_all_SOURCE_DIR}\"
    RESULT_VARIABLE   git_return
    OUTPUT_VARIABLE   vidtk_git_hash)
  execute_process(
    COMMAND           \"${GIT_EXECUTABLE}\"
                      rev-parse
                      --short
                      HEAD
    WORKING_DIRECTORY \"${vidtk_all_SOURCE_DIR}\"
    RESULT_VARIABLE   git_return
    OUTPUT_VARIABLE   vidtk_git_hash_short)
  execute_process(
    COMMAND           \"${GIT_EXECUTABLE}\"
                      diff
                      --no-ext-diff
                      --quiet
                      --exit-code
    WORKING_DIRECTORY \"${vidtk_all_SOURCE_DIR}\"
    RESULT_VARIABLE   git_return)

  string(STRIP \"\${vidtk_git_hash}\" vidtk_git_hash)
  string(STRIP \"\${vidtk_git_hash_short}\" vidtk_git_hash_short)

  if (git_return)
    set(vidtk_git_dirty \"dirty\")
  endif (git_return)
endif ()
")
else ()
  message("Unable to determine the git hash of the build.")
  set( vidtk_git_hash "EXTERNAL" )
  set( vidtk_git_hash_short "EXTERNAL" )
  set( vidtk_git_dirty "EXTERNAL" )
endif ()

set(version_script
  "${CMAKE_CURRENT_BINARY_DIR}/version-script.cmake")

# Clear the file
file(WRITE "${version_script}"
  "# Configure script for version.h\n")

# Add in the configure code
file(APPEND "${version_script}"
  "${configure_code}")

# Configure the file
file(APPEND "${version_script}" "
configure_file(
  \"${CMAKE_CURRENT_SOURCE_DIR}/version.h.in\"
  \"${CMAKE_CURRENT_BINARY_DIR}/version.h.tmp\"
  @ONLY)\n")

# Copy the file to the result if it is different
file(APPEND "${version_script}" "
execute_process(
  COMMAND \"${CMAKE_COMMAND}\"
          -E copy_if_different
          \"${CMAKE_CURRENT_BINARY_DIR}/version.h.tmp\"
          \"${CMAKE_CURRENT_BINARY_DIR}/version.h\")\n")

# Make the command
add_custom_command(
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/version.noexist.h"
          "${CMAKE_CURRENT_BINARY_DIR}/version.h"
  COMMAND "${CMAKE_COMMAND}"
          -D vidtk_version_major=${vidtk_VERSION_MAJOR}
          -D vidtk_version_minor=${vidtk_VERSION_MINOR}
          -D vidtk_version_patch=${vidtk_VERSION_PATCH}
          -D vidtk_version=${vidtk_VERSION}
          -D VIDTK_BUILT_FROM_GIT=${VIDTK_BUILT_FROM_GIT}
          -D vidtk_git_hash=${vidtk_git_hash}
          -D vidtk_git_hash_short=${vidtk_git_hash_short}
          -D vidtk_git_dirty=${vidtk_git_dirty}
          -P "${version_script}"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
          "${version_script}"
  WORKING_DIRECTORY
          "${CMAKE_CURRENT_BINARY_DIR}"
          COMMENT "Creating version.h file")
add_custom_target(configure-version.h ALL
  DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/version.h")

if(USE_ANGEL_FIRE)
  set_source_files_properties(
    dir_string_reader_impl.cxx
    PROPERTIES COMPILE_FLAGS "-DUSE_ANGEL_FIRE")
endif()

###
#
# The following block is related to the utilities plugin library.
# Algorithmic utility that is provided by 3rd party vendors belongs here.
#
# Note, this libray should not be linked PUBLIC by any other library in
# vidtk. The intent is to minimize dependency leakage to the outside world.
#
###
set( vidtk_utilites_plugin_sources )
set( utilities_plugin_public_links )
set( utilities_plugin_private_links )
if( VIDTK_ENABLE_OPENCV )
  # Set the name of the plugin library here, because now we have one
  # Note, this behaviour prevents an empty library which won't get built on Windows
  set( vidtk_utilites_plugin_lib vidtk_utilities_plugins CACHE STRING "utilities plugin lib name" FORCE)
  list( APPEND utilities_plugin_public_links vcl vgl vil vidtk_logger ${OpenCV_LIBS} )
  list( APPEND vidtk_utilites_plugin_sources
    vxl_to_cv_converters.h
    kwocv_utility.h kwocv_utility.cxx
    )
endif()

if( VIDTK_ENABLE_VLFEAT )
  # Set the name of the plugin library here, because now we have one
  # Note, this behaviour prevents an empty library which won't get built on Windows
  set( vidtk_utilites_plugin_lib vidtk_utilities_plugins CACHE STRING "utilities plugin lib name" FORCE)
  list( APPEND utilities_plugin_public_links vcl vgl vil ${VLFeat_LIBRARY} )
  list( APPEND vidtk_utilites_plugin_sources
    ${vidtk_utilities_sources}
    vxl_to_vl_converters.h
    )
endif()


# If we set the library, proceed to create and link it.
if (vidtk_utilites_plugin_lib)
  add_library( ${vidtk_utilites_plugin_lib} ${vidtk_utilites_plugin_sources} )
  set_target_properties(vidtk_utilities_plugins PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties( ${vidtk_utilites_plugin_lib} PROPERTIES
    VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
    )
  target_link_libraries( ${vidtk_utilites_plugin_lib}
    PUBLIC ${utilities_plugin_public_links}
    PRIVATE ${utilities_plugin_private_links}
    )
  install( TARGETS vidtk_utilities_plugins EXPORT vidtk
    RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
    )
  export( TARGETS vidtk_utilities_plugins APPEND FILE "${VIDTK_EXPORT_FILE}" )
endif()

###
#
# The following block is related to the utilities_no_process library.
#
###
set( vidtk_utilities_sources
  "${CMAKE_CURRENT_BINARY_DIR}/version.h"
  aoi.h                             aoi.cxx
  unchecked_return_value.h          unchecked_return_value.cxx
  checked_bool.h                    checked_bool.cxx
  config_block.h                    config_block.cxx
                                    config_block.txx
  config_block_parser.h             config_block_parser.cxx
  config_block_utils.h              config_block_utils.cxx
  compute_transformations.h         compute_transformations.cxx
  frame_downsampler.h               frame_downsampler.cxx
  frame_rate_estimator.h            frame_rate_estimator.cxx
  folder_manipulation.h             folder_manipulation.cxx
  fuzzy_compare.h
  geo_lat_lon.h                     geo_lat_lon.cxx
  geo_UTM.h                         geo_UTM.cxx
  geo_MGRS.h                        geo_MGRS.cxx
  geo_coordinate.h                  geo_coordinate.cxx
  vsl/geo_io.h                      vsl/geo_io.cxx
  property_map.h                    property_map.cxx
  timestamp.h                       timestamp.cxx
  vsl/timestamp_io.h                vsl/timestamp_io.cxx
  format_block.h                    format_block.cxx
  buffer.h
  ring_buffer.h                     ring_buffer.txx
                                    Templates/ring_buffer_instances.cxx
  barcode_factory.h                 barcode_factory.txx
                                    Templates/barcode_factory_instances.cxx
  timestamp_image.h                 timestamp_image.txx
                                    Templates/timestamp_image_instances.cxx
  oc_timestamp_hack.h               oc_timestamp_hack.cxx
  image_histogram.h                 image_histogram.cxx
  image_histogram.txx               Templates/image_histogram_instances.cxx
  vsl/image_histogram_io.h          vsl/image_histogram_io.cxx
  greedy_assignment.h               greedy_assignment.cxx
  compute_transformations.h         compute_transformations.cxx
  ffmt.h                            ffmt.cxx
  video_metadata.h                  video_metadata.cxx
  video_metadata_util.h             video_metadata_util.cxx
  videoname_prefix.h                videoname_prefix.cxx
  homography.h                      homography.cxx
  homography_util.h                 homography_util.cxx
  interpolate_corners_from_shift.h  interpolate_corners_from_shift.cxx
  compute_gsd.h                     compute_gsd.cxx
  video_modality.h                  video_modality.cxx
  training_thread.h                 training_thread.cxx
  base_reader_writer.h              base_reader_writer.cxx
  group_data_reader_writer.h        group_data_reader_writer.cxx
  timestamp_reader_writer.h
  video_metadata_reader_writer.h    video_metadata_vector_reader_writer.h
  video_modality_reader_writer.h
  gsd_reader_writer.h
  shot_break_flags_reader_writer.h
  image_view_reader_writer.h
  large_file_ifstream.h             large_file_ifstream.cxx
  large_file_ofstream.h             large_file_ofstream.cxx
  kw_archive_index_reader.h         kw_archive_index_reader.cxx
  kw_archive_index_writer.h         kw_archive_index_writer.cxx
                                    Templates/kw_archive_index_writer_instances.cxx
  kw18_timestamp_parser.h           kw18_timestamp_parser.cxx
  token_expansion.h                 token_expansion.cxx
  thread_util.h                     thread_util.cxx
  paired_buffer.h                   paired_buffer.txx
  Templates/paired_buffer_instances.cxx
  uuid_able.h uuid_able.cxx
  polygon_centroid.h                polygon_centroid.cxx
  point_view_to_region.h
  external_settings.h               external_settings.cxx
  string_to_vector.h
  klv_to_metadata.h                 klv_to_metadata.cxx

  write_videometa_kml.h             write_videometa_kml.cxx
  gsd_file_source.h                 gsd_file_source.cxx

  Templates/vsl_vector_io+vnl_vector_fixed+double.2--.cxx
  Templates/vsl_vector_io+vector+char--.cxx
)

set( utilities_no_process_private_links
  vidtk_logger vnl_io vpgl rrel ${vidtk_utilites_plugin_lib}
  ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY}
  )
set( utilities_no_process_public_links
  vidtk_geographic vidtk_klv
  vil_io vgl_algo vil vnl vsl vcl vul
  ${Boost_THREAD_LIBRARY} ${Boost_DATE_TIME_LIBRARY}
  )

add_library( vidtk_utilities_no_process ${vidtk_utilities_sources} )
set_target_properties( vidtk_utilities_no_process PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
)

target_link_libraries( vidtk_utilities_no_process
  PUBLIC ${utilities_no_process_public_links}
  PRIVATE ${utilities_no_process_private_links}
  )
install( TARGETS vidtk_utilities_no_process EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
export( TARGETS vidtk_utilities_no_process APPEND FILE "${VIDTK_EXPORT_FILE}" )


###
#
# The following block is related to the utilities library.
#
###
set( vidtk_utilities_process_sources
  apply_function_to_vector_process.h apply_function_to_vector_process.txx
  apply_functor_to_vector_process.h apply_functor_to_vector_process.txx
  async_observer_process.h          async_observer_process.txx
  deserialize_process.h             deserialize_process.txx
  downsampling_process.h            downsampling_process.cxx
  extract_matrix_process.h          extract_matrix_process.txx
                                    Templates/extract_matrix_process_instances.cxx
  file_stream_process.h             file_stream_process.cxx
  dir_string_reader_impl.h          dir_string_reader_impl.cxx
  file_string_reader_impl.h         file_string_reader_impl.cxx
  filter_video_metadata_process.h   filter_video_metadata_process.cxx
  holder_process.h                  holder_process.txx
                                    Templates/holder_process_instances.cxx
  homography_reader_process.h       homography_reader_process.cxx
  homography_holder_process.h       homography_holder_process.cxx
  homography_reader_or_generator_process.h      homography_reader_or_generator_process.cxx
  homography_writer_process.h       homography_writer_process.cxx
  homography_scoring_process.h      homography_scoring_process.cxx
  horizon_detection_functions.h     horizon_detection_functions.cxx
  kw_archive_writer_process.h       kw_archive_writer_process.cxx
                                    Templates/kw_archive_writer_process_instances.cxx
  list_writer_process.h             list_writer_process.txx
  merge_n_vectors_process.h
  metadata_reader_process.h         metadata_reader_process.cxx
  ring_buffer_process.h             ring_buffer_process.txx
                                    Templates/ring_buffer_process_instances.cxx
  serialize_process.h               serialize_process.txx
  split_vector_process.h            split_vector_process.txx
  string_reader_process.h           string_reader_process.cxx
  tcp_string_reader_impl.h          tcp_string_reader_impl.cxx
  timestamp_generator.h             timestamp_generator.cxx
  timestamp_generator_process.h     timestamp_generator_process.cxx
  transform_timestamp_process.h     transform_timestamp_process.txx
                                    Templates/transform_timestamp_process_instances.cxx
  transform_vidtk_homography_process.h transform_vidtk_homography_process.txx
                                    Templates/transform_vidtk_homography_process_instances.cxx
  project_vidtk_homography_process.h project_vidtk_homography_process.txx
                                    Templates/project_vidtk_homography_process_instances.cxx
  interpolate_corners_from_shift_process.h interpolate_corners_from_shift_process.txx
                                    Templates/interpolate_corners_from_shift_process_instances.cxx

  base_io_process.h                  base_io_process.cxx
  base_reader_process.h              base_reader_process.cxx
  base_writer_process.h              base_writer_process.cxx
  tagged_data_writer_process.h       tagged_data_writer_process.cxx
  tagged_data_reader_process.h       tagged_data_reader_process.cxx

  shell_comments_filter.h
  blank_line_filter.h

  queue_process.h                    queue_process.txx
                                     Templates/queue_process_instances.cxx
  kw_archive_writer_process.h        kw_archive_writer_process.cxx
  paired_buffer_process.h            paired_buffer_process.txx
  Templates/paired_buffer_process_instances.cxx
  video_metadata_writer_process.h    video_metadata_writer_process.cxx
  rescale_tracks.h                   rescale_tracks.cxx
  rescale_tracks_process.h           rescale_tracks_process.cxx
  track_demultiplexer.h              track_demultiplexer.cxx
  track_demultiplexer_process.h      track_demultiplexer_process.cxx
  split_string_process.h             split_string_process.cxx
  metadata_f2f_homography_process.h  metadata_f2f_homography_process.cxx
)

set( utilities_private_links )
set( utilities_public_links
  vidtk_utilities_no_process vidtk_process_framework
  vidtk_tracking_data vidtk_pipeline_framework ${Boost_THREAD_LIBRARY}
  )

add_library( vidtk_utilities ${vidtk_utilities_process_sources} )
set_target_properties( vidtk_utilities PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
)
if( VIDTK_ENABLE_OPENCV )
  list( APPEND utilities_private_links ${OpenCV_LIBS} )
endif()

target_link_libraries( vidtk_utilities
  PUBLIC ${utilities_public_links}
  PRIVATE ${utilities_private_links}
  )
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
