set( vidtk_tracking_gui_sources
  image_object.h                         image_object.cxx
  pvo_track.h                            pvo_track.cxx
  synced_viewer2D_manager.h              synced_viewer2D_manager.cxx
  synced_viewer2D_tableau.h              synced_viewer2D_tableau.cxx
  synced_viewer2D_tableau_sptr.h
  track.h                                track.cxx
  vgui_process.h                         vgui_process.cxx
)

add_library( vidtk_tracking_gui ${vidtk_tracking_gui_sources} )
set_target_properties( vidtk_tracking_gui PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
)
target_link_libraries( vidtk_tracking_gui
                       vgui vidtk_tracking )
install( TARGETS vidtk_tracking_gui EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
export( TARGETS vidtk_tracking_gui APPEND FILE "${VIDTK_EXPORT_FILE}")
