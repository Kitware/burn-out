
# DARTS needs checksum disabled. Default is OFF, meaning checksum is enabled.
OPTION( KLV_0601_CHECKSUM_DISABLE "Disable KLV0601 checksums for DARTS video source" OFF)
mark_as_advanced( KLV_0601_CHECKSUM_DISABLE )

set( vidtk_klv_sources
  klv_parse.h                       klv_parse.cxx
  klv_data.h                        klv_data.cxx
  klv_key.h                         klv_key.cxx
  klv_0601_traits.h
  klv_0601.h                        klv_0601.cxx
  klv_0104.h                        klv_0104.cxx
)

add_library( vidtk_klv ${vidtk_klv_sources} )
set_target_properties( vidtk_klv PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
)

if ( KLV_0601_CHECKSUM_DISABLE )
   set_target_properties( vidtk_klv PROPERTIES
                          COMPILE_DEFINITIONS KLV_0601_CHECKSUM_DISABLE )
endif()

target_link_libraries( vidtk_klv vidtk_logger )

install( TARGETS vidtk_klv EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
export( TARGETS vidtk_klv APPEND FILE "${VIDTK_EXPORT_FILE}")
