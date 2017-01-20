#
# Tracking data io
#

include( GenerateExportHeader )

set( vidtk_tracking_data_io_sources
  ${tracking_io_tinyxml_src}
  ${tracking_io_track_oracle_src}
  active_tracks_generator.h              active_tracks_generator.cxx
  active_tracks_reader.h                 active_tracks_reader.cxx

  image_object_reader_process.h          image_object_reader_process.cxx
  image_object_reader_interface.h        image_object_reader_interface.cxx
  image_object_reader.h                  image_object_reader.cxx

  image_object_writer_process.h          image_object_writer_process.cxx
  image_object_writer.h                  image_object_writer.cxx

  raw_descriptor_writer_process.h        raw_descriptor_writer_process.cxx
  raw_descriptor_writer.h                raw_descriptor_writer.cxx
  raw_descriptor_writer_interface.h      raw_descriptor_writer_interface.cxx

  tot_writer.h                           tot_writer.cxx
  tot_writer_process.h                   tot_writer_process.cxx

  track_reader_process.h                 track_reader_process.cxx
  track_reader.h                         track_reader.cxx
  track_reader_interface.h               track_reader_interface.cxx

  track_writer_process.h                 track_writer_process.cxx
  track_writer.h                         track_writer.cxx
  track_writer_interface.h               track_writer_interface.cxx
)

add_library( vidtk_tracking_data_io ${vidtk_tracking_data_io_sources} )

set_target_properties( vidtk_tracking_data_io PROPERTIES
  VERSION ${vidtk_VERSION}
  SOVERSION ${vidtk_VERSION}
)

target_link_libraries( vidtk_tracking_data_io
  vidtk_tracking_data
  vidtk_utilities
  plugin_loader
  vnl vgl
)


install( TARGETS vidtk_tracking_data_io EXPORT vidtk
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
export(TARGETS vidtk_tracking_data_io APPEND FILE "${VIDTK_EXPORT_FILE}")

# ==================================================================
###
# Plugins for track readers and writers
#
list(APPEND PLUGIN_TRACKING_DATA_LIBRARIES
  vidtk_tracking_data
  ${Boost_FILESYSTEM_LIBRARY}
  vsl vil vul vgl
  )

vidtk_add_plugin(
  NAME vidtk_track_io_plugin
  SOURCES track_io_plugin.cxx
    track_reader_interface.h               track_reader_interface.cxx
    track_reader_kw18.h                    track_reader_kw18.cxx
    track_reader_vsl.h                     track_reader_vsl.cxx

    track_writer_interface.h               track_writer_interface.cxx
    track_writer_kw18_col.h                track_writer_kw18_col.cxx
    track_writer_vsl.h                     track_writer_vsl.cxx
  LINK_LIBRARIES ${PLUGIN_TRACKING_DATA_LIBRARIES}
  )

set_target_properties( vidtk_track_io_plugin PROPERTIES
  DEFINE_SYMBOL vidtk_track_io_plugin_EXPORTS )

###
#
if ( VIDTK_ENABLE_SHAPELIB )
  vidtk_add_plugin(
    NAME vidtk_track_io_shapelib_plugin
    SOURCES shapelib_io_plugin.cxx
      track_writer_interface.h             track_writer_interface.cxx
      shape_file_writer.h                  shape_file_writer.cxx
      track_writer_shape.h                 track_writer_shape.cxx
    LINK_LIBRARIES ${SHAPELIB_LIBRARY} vidtk_utilities_no_process vidtk_tracking_data
    )

set_target_properties( vidtk_track_io_shapelib_plugin PROPERTIES
  DEFINE_SYMBOL vidtk_track_io_plugin_EXPORTS )

endif()


###
#
if (USE_CPPDB_BACKEND)
  vidtk_add_plugin(
    NAME vidtk_track_io_db_plugin
    SOURCES track_io_db_plugin.cxx
      track_writer_interface.h             track_writer_interface.cxx
      track_writer_db.h                    track_writer_db.cxx
      track_reader_interface.h             track_reader_interface.cxx
      track_reader_db.h                    track_reader_db.cxx
  LINK_LIBRARIES vidtk_tracking_data vidtk_database
  )

set_target_properties( vidtk_track_io_db_plugin PROPERTIES
  DEFINE_SYMBOL vidtk_track_io_plugin_EXPORTS )

endif()

###
#
vidtk_add_plugin(
  NAME vidtk_image_object_io_plugin
  SOURCES image_object_io_plugin.cxx
    image_object_reader_interface.h        image_object_reader_interface.cxx
    image_object_reader_default.h          image_object_reader_default.cxx
    image_object_reader_vsl.h              image_object_reader_vsl.cxx
    image_object_reader_kw18.h             image_object_reader_kw18.cxx
    image_object_writer.h                  image_object_writer.cxx
    image_object_writer_vsl.h              image_object_writer_vsl.cxx
    image_object_writer_kw18.h             image_object_writer_kw18.cxx
    image_object_writer_default.h          image_object_writer_default.cxx
  LINK_LIBRARIES vidtk_tracking_data vidtk_utilities_no_process vsl vbl
  )

set_target_properties( vidtk_image_object_io_plugin PROPERTIES
  DEFINE_SYMBOL vidtk_track_io_plugin_EXPORTS )

###
#
if( VIDTK_ENABLE_PROTOBUF )

  vidtk_add_plugin(
    NAME vidtk_image_object_io_protobuf_plugin
    SOURCES image_object_io_protobuf_plugin.cxx
      track_reader_interface.h               track_reader_interface.cxx
      image_object_reader_interface.h        image_object_reader_interface.cxx
      image_object_reader_protobuf.h         image_object_reader_protobuf.cxx
      track_writer_interface.h               track_writer_interface.cxx
      image_object_writer.h                  image_object_writer.cxx
      image_object_writer_protobuf.h         image_object_writer_protobuf.cxx
    LINK_LIBRARIES vidtk_protobuf vidtk_utilities_no_process
    )

set_target_properties( vidtk_image_object_io_protobuf_plugin PROPERTIES
  DEFINE_SYMBOL vidtk_track_io_plugin_EXPORTS )

endif()

###
#
vidtk_add_plugin(
  NAME vidtk_raw_descriptor_io_plugin
  SOURCES raw_descriptor_io_plugin.cxx
    track_writer_interface.h               track_writer_interface.cxx
    raw_descriptor_writer_interface.h      raw_descriptor_writer_interface.cxx
    raw_descriptor_writer_csv.h            raw_descriptor_writer_csv.cxx
    raw_descriptor_writer_json.h           raw_descriptor_writer_json.cxx
    raw_descriptor_writer_xml.h            raw_descriptor_writer_xml.cxx
  LINK_LIBRARIES vidtk_tracking_data vidtk_utilities_no_process
  )

set_target_properties( vidtk_raw_descriptor_io_plugin PROPERTIES
  DEFINE_SYMBOL vidtk_track_io_plugin_EXPORTS )

###
#
generate_export_header( vidtk_track_io_plugin
  EXPORT_MACRO_NAME PLUGIN_EXPORT
  EXPORT_FILE_NAME plugin_export.h
  )
