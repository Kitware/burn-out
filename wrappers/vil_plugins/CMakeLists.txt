find_package(VXL REQUIRED)
unset(VXL_CMAKE_DOXYGEN_DIR)
include(${VXL_CMAKE_DIR}/UseVXL.cmake)
include_directories(SYSTEM ${VXL_VIL_INCLUDE_DIR})
include_directories(SYSTEM ${VXL_VUL_INCLUDE_DIR})

set(vil_plugin_link_libraries)
set(vil_plugin_source vil_plugin_loader.h vil_plugin_loader.cxx)

if(VIDTK_HAS_GDAL)
  set(HAS_VIL_GDAL True)
  include_directories(SYSTEM ${GDAL_INCLUDE_DIR})
  list(APPEND vil_plugin_source
    file_formats/cpl_vsi_vil.h
    file_formats/cpl_vsi_vil.cxx
    file_formats/vil_gdal_image.cxx
  )
endif()

find_package(LTIDSDK 8.5)
if(LTIDSDK_FOUND OR GDAL_FOUND)
  list(APPEND vil_plugin_source
    file_formats/vil_nitf_engrda.h
    file_formats/vil_nitf_engrda.cxx
    file_formats/vil_nitf_metadata_parser.h
    file_formats/vil_nitf_metadata_parser.cxx
  )
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/vil_plugins_config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vil_plugins_config.h
)

add_library(vidtk_vil_plugins ${vil_plugin_source})
target_link_libraries(vidtk_vil_plugins vil vul)
if(LTIDSDK_FOUND)
  list(APPEND vil_plugin_link_libraries ${LTIDSDK_LIBRARIES})
endif()
if(GDAL_FOUND)
  list(APPEND vil_plugin_link_libraries ${GDAL_LIBRARY})
endif()
if(LTIDSDK_FOUND OR GDAL_FOUND)
  list(APPEND vil_plugin_link_libraries ${Boost_LIBRARIES})
endif()

target_link_libraries(vidtk_vil_plugins ${vil_plugin_link_libraries})
set_target_properties(vidtk_vil_plugins PROPERTIES
  VERSION ${vidtk_VERSION} SOVERSION ${vidtk_VERSION}
  )

include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)

add_executable(vil_convert vil_convert.cxx)
target_link_libraries(vil_convert vul vil vidtk_vil_plugins)

install(TARGETS vidtk_vil_plugins vil_convert EXPORT vidtk
  RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/vil_plugins_config.h
  DESTINATION include/vil_plugins
)
export( TARGETS vidtk_vil_plugins APPEND FILE "${VIDTK_EXPORT_FILE}")
