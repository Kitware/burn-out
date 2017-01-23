# Tests that don't require any arguments at runtime
#
set( TRACK_IO_FILES )
set( optional_libraries )
if (VIDTK_ENABLE_SHAPELIB)
  set(SHAPELIB_DEPENDENT_SOURCES test_shape_file_writer.cxx)
  list( APPEND     TRACK_IO_FILES
    shape_file_writer.h                  shape_file_writer.cxx
    track_writer_shape.h                 track_writer_shape.cxx
  )

  find_package(SHAPELIB REQUIRED)
  include_directories(SYSTEM ${SHAPELIB_INCLUDE_DIR})
  list( APPEND optional_libraries ${SHAPELIB_LIBRARY} )

endif()

if (VIDTK_ENABLE_PROTOBUF)
  set(feature_definitions "USE_PROTOBUF")
endif()

  list( APPEND     TRACK_IO_FILES
    track_reader_kw18.h                    track_reader_kw18.cxx
    track_reader_vsl.h                     track_reader_vsl.cxx

    track_writer_kw18_col.h                track_writer_kw18_col.cxx
    track_writer_vsl.h                     track_writer_vsl.cxx
  )

if (USE_CPPDB_BACKEND)
  list( APPEND     TRACK_IO_FILES
    track_writer_interface.h             track_writer_interface.cxx
    track_writer_db.h                    track_writer_db.cxx
    track_reader_interface.h             track_reader_interface.cxx
    track_reader_db.h                    track_reader_db.cxx
  )
endif()

set( no_argument_test_sources
  test_frame_data.cxx
  test_image_object_serialization.cxx
  test_track_transformation_functions.cxx
  test_fg_matcher.cxx
  test_raw_descriptor.cxx
  test_raw_descriptor_writer.cxx
)

# Tests that take the data directory as the only argument at runtime
#
set( data_argument_test_sources
  test_mit_reader.cxx
  test_kw18_reader.cxx
  test_vsl_reader.cxx
  test_track_reader_process.cxx
  test_track_reader.cxx
  test_track_util.cxx
  test_track.cxx
  test_track_view.cxx
  test_active_tracks_generator.cxx
  ${track_oracle_tests}
  ${SHAPELIB_DEPENDENT_SOURCES}
)

if (VIDTK_ENABLE_TinyXML)
  list( APPEND     data_argument_test_sources
    test_viper_reader.cxx
  )
endif()

foreach( file IN LISTS TRACK_IO_FILES )
  list( APPEND TRACK_IO_FILES_WITH_PATH
    ${CMAKE_SOURCE_DIR}/library/tracking_data/io/${file}
  )
endforeach()

if (USE_CPPDB_BACKEND)
  list( APPEND data_argument_test_sources
    test_reader_db.cxx
    test_writer_db.cxx
    )
  list( APPEND optional_libraries vidtk_database)
endif()


create_test_sourcelist( test_sources
  test_driver.cxx

  # List of source files
  ${no_argument_test_sources}
  ${data_argument_test_sources}
  ${custom_argument_test_sources}
)

add_executable( vidtk_test_tracking_data
  ${test_sources}
  ${TRACK_IO_FILES_WITH_PATH}
)

target_link_libraries( vidtk_test_tracking_data vidtk_tracking_data
                       vidtk_tracking_data_io vidtk_video_io
                       vgl_algo
                       testlib vnl vul vpl
                       ${optional_libraries})

set_target_properties( vidtk_test_tracking_data PROPERTIES
  COMPILE_DEFINITIONS  "${feature_definitions}"
)

# Create ADD_TEST lines for some tests automatically.  Tests taking
# special arguments will be specified manually later.

foreach( filename ${no_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_tracking_data_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_tracking_data> ${filebase} )
endforeach( filename )

foreach( filename ${data_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_tracking_data_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_tracking_data> ${filebase}
                    ${vidtk_tests_SOURCE_DIR}/data )
endforeach( filename )
