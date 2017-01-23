#
# could merge with utils
#

include_directories(SYSTEM ${KWSYS_INCLUDE_DIR})

set( sources
  plugin_manager.h                  plugin_manager.cxx
  plugin_factory.h                  plugin_factory.cxx
  plugin_config_util.h              plugin_config_util.cxx
  )

if (WIN32)

  set(libdir lib) # MODULES are always in lib
  set(destdir lib)
  set(path_sep ";")
  set(path_prefix "L")
  foreach( config IN LISTS CMAKE_CONFIGURATION_TYPES)
    if (NOT module_output_path)
      set(module_output_path "${CMAKE_BINARY_DIR}/${destdir}/modules/${config}")
    else()
      set(module_output_path "${module_output_path}${path_sep}${CMAKE_BINARY_DIR}/${destdir}/modules/${config}")
    endif()
  endforeach()

else ()

  set(libdir lib${LIB_SUFFIX})
  set(destdir lib)
  set(path_sep ":")
  set(path_prefix)
  set(module_output_path  "${CMAKE_BINARY_DIR}/${destdir}/modules")

endif ()

set(module_build_options
  "LIBRARY_SUFFIX=\"${CMAKE_SHARED_MODULE_SUFFIX}\"")

set( vidtk_default_module_paths
    "${CMAKE_INSTALL_PREFIX}/${libdir}/modules")

option(VIDTK_USE_BUILD_MODULE_PATHS "Whether to use the build tree for the default module path" ON)
mark_as_advanced(VIDTK_USE_BUILD_MODULE_PATHS)
if (VIDTK_USE_BUILD_MODULE_PATHS)
  set(vidtk_default_module_paths
    "${vidtk_default_module_paths}${path_sep}${module_output_path}")
endif ()

set(VIDTK_DEFAULT_MODULE_PATHS "${vidtk_default_module_paths}"
  CACHE STRING "The default paths for module scanning")
mark_as_advanced( VIDTK_DEFAULT_MODULE_PATHS )

set( vidtk_library_options
  "DEFAULT_MODULE_PATHS=\"${VIDTK_DEFAULT_MODULE_PATHS}\"")

#
# This is all to get around the ';' as the path separator.  CMake will
# split it into two elements when we want it to be ignored.
#
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/module-paths.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/module-paths.h" )

set_source_files_properties(plugin_manager.cxx
  PROPERTIES
  COMPILE_DEFINITIONS "${module_build_options}" )

add_library( plugin_loader ${sources} )
target_link_libraries( plugin_loader
  PRIVATE      ${Boost_SYSTEM_LIBRARY}
               ${Boost_THREAD_LIBRARY}
               ${Boost_FILESYSTEM_LIBRARY}
               vidtk_utilities_no_process
               vidtk_logger
               vidtksys
)

export( TARGETS plugin_loader APPEND FILE "${VIDTK_EXPORT_FILE}" )

# Add tool
add_executable( pluginopedia pluginopedia.cxx )
target_link_libraries( pluginopedia
  PRIVATE      plugin_loader
               vidtksys
               vul
  )

install( TARGETS plugin_loader pluginopedia
  EXPORT vidtk
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  )
