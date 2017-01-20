set( vidtk_track_classifiers_sources
)

set( vidtk_classifiers_sources
  hashed_image_classifier.h                  hashed_image_classifier.txx
  hashed_image_classifier_process.h          hashed_image_classifier_process.txx
)

aux_source_directory(Templates vidtk_classifiers_sources)

add_library( vidtk_classifiers ${vidtk_classifiers_sources} )

set_target_properties( vidtk_classifiers PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
)

target_link_libraries( vidtk_classifiers
  vidtk_utilities vidtk_process_framework vil_algo vil_io vil vnl vgl_algo vgl )
install( TARGETS vidtk_classifiers EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
export( TARGETS vidtk_classifiers APPEND FILE "${VIDTK_EXPORT_FILE}")

add_library( vidtk_track_classifiers ${vidtk_track_classifiers_sources} )
set_target_properties( vidtk_track_classifiers PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
)
target_link_libraries( vidtk_track_classifiers
  vidtk_learning vidtk_descriptor vidtk_tracking_data vbl vnl )
install( TARGETS vidtk_track_classifiers EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
export( TARGETS vidtk_track_classifiers APPEND FILE "${VIDTK_EXPORT_FILE}")
