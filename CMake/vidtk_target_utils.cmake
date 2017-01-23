#
# CMake support for building vidtk targets
#
#

####
# This function creates a target for a loadable plugin.
#
# NAME: The name of the plugin to create.
# SOURCES: List of the sources to create the library.
# LINK_LIBRARIES: List of the libraries the plugin will privately link against.
#
function( vidtk_add_plugin )
  include(CMakeParseArguments)
  set(options)
  set(oneValueArgs NAME)
  set(multiValueArgs SOURCES LINK_LIBRARIES)
  cmake_parse_arguments(PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  message( STATUS "Making library \"${PLUGIN_NAME}\"" )

  add_library( ${PLUGIN_NAME} MODULE ${PLUGIN_SOURCES} )

  target_link_libraries( ${PLUGIN_NAME}
    PRIVATE              plugin_loader ${PLUGIN_LINK_LIBRARIES}
    )

  set_target_properties( ${PLUGIN_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/modules
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/modules
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/modules
    PREFIX               ""
    )

  if (NOT APPLE)
    set_target_properties( ${PLUGIN_NAME} PROPERTIES
      VERSION              ${vidtk_VERSION}
      SOVERSION            ${vidtk_VERSION}
      )
  endif()

  install( TARGETS ${PLUGIN_NAME}
     ARCHIVE DESTINATION lib/modules
     LIBRARY DESTINATION lib/modules
     RUNTIME DESTINATION bin/modules
    )

endfunction()
