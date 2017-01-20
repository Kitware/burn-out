set( vidtk_tracking_data_sources
  amhi_data.h                                   amhi_data.cxx
  frame_data.h                                  frame_data.cxx
  fg_matcher.h                                  fg_matcher.cxx
  gui_frame_info.h                              gui_frame_info.cxx
  gui_track_signal.h                            gui_track_signal.cxx
  image_border.h
  image_object.h                                image_object.cxx
  image_object_util.h
  pixel_feature_array.h
  pvo_probability.h                             pvo_probability.cxx
  raw_descriptor.h                              raw_descriptor.cxx
  shot_break_flags.h
  track.h                                       track.cxx
  crop_track_to_aoi.h                           crop_track_to_aoi.cxx
  track_sptr.h
  track_state.h                                 track_state.cxx
  track_attributes.h                            track_attributes.cxx
  track_state_util.h                            track_state_util.cxx
  track_state_attributes.h                      track_state_attributes.cxx
  track_state_util.h                            track_state_util.cxx
  track_view.h                                  track_view.cxx
  track_util.h                                  track_util.cxx
  tracking_keys.h                               tracking_keys.cxx
  vsl/image_object_io.h                         vsl/image_object_io.cxx
  vsl/track_io.h                                vsl/track_io.cxx
  vsl/vil_image_resource_sptr_io.h              vsl/vil_image_resource_sptr_io.cxx
)
AUX_SOURCE_DIRECTORY(Templates vidtk_tracking_data_sources)

add_library( vidtk_tracking_data ${vidtk_tracking_data_sources} )

set_target_properties( vidtk_tracking_data PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
)

target_link_libraries( vidtk_tracking_data
  PUBLIC vidtk_utilities_no_process vsl vil vnl vgl
  PRIVATE vgl_algo vgl_io vnl_io vil_io ${Boost_THREAD_LIBRARY}
  )

install( TARGETS vidtk_tracking_data EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
export(TARGETS vidtk_tracking_data APPEND FILE "${VIDTK_EXPORT_FILE}")

add_subdirectory(io)
