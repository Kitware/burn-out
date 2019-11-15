project( vidtk_apps )

# =================================================================
# Helper Function Definitions
# =================================================================

# Default command used to build and install a VIDTK tool.
#
# This command should be used by default, unless a non-standard
# CMake install option is required. This function automatically
# adds the vidtk binary prefix to the generated executable name in
# addition to building, linking, and installing the executable in
# a single CMake function call. Optionally, the third argument
# can contain any variable definitions to be passed to the tool.
#
# Example usages:
#
# add_vidtk_tool( example_tool             <-- Binary name
#                 example_tool.cxx         <-- Source file
#                 {VIDTK_LIBRARIES} )      <-- Linked libraries
#
# add_vidtk_tool( example_tool_2           <-- Binary name
#                 example_tool_2.cxx       <-- Source file
#                 "-DNOGUI"                <-- Optional defines
#                 {VIDTK_LIBRARIES} )      <-- Linked libraries
function( add_vidtk_tool bin_name source_file install_prop )
  add_executable( ${VIDTK_BINARY_PREFIX}${bin_name}
    ${source_file} )

  if( install_prop MATCHES "-D*" )
    set_target_properties( ${VIDTK_BINARY_PREFIX}${bin_name}
      PROPERTIES COMPILE_FLAGS ${install_prop} )

    target_link_libraries( ${VIDTK_BINARY_PREFIX}${bin_name}
      ${ARGN} )
  else()
    target_link_libraries( ${VIDTK_BINARY_PREFIX}${bin_name}
      ${install_prop} ${ARGN} )
  endif()

  install( TARGETS ${VIDTK_BINARY_PREFIX}${bin_name}
    RUNTIME DESTINATION bin )
endfunction()

# Conditionally add tool
function( add_vidtk_tool_if FLAG bin_name source_file install_prop )
  if (${FLAG})
    add_vidtk_tool( ${bin_name} ${source_file} ${install_prop} ${ARGN})
  endif()
endfunction()

# Custom link command which appends the VIDTK binary prefix
#
# This function can be used to add an additional link dependency if
# some conditional is met (such if we are building with a certain
# external library).
function( link_tool_targets bin_name )

  target_link_libraries( ${VIDTK_BINARY_PREFIX}${bin_name}
    ${ARGN} )

endfunction()

include_directories( ${vidtk_SOURCE_DIR} ${vidtk_BINARY_DIR} )

if( VIDTK_ENABLE_OPENCV )
  add_vidtk_tool( remove_metadata_burnin
    remove_metadata_burnin.cxx
    vidtk_video_io vidtk_object_detectors
    vidtk_pipeline_framework vidtk_pipelines )

  if( VIDTK_ENABLE_QT )
    if( VIDTK_ENABLE_MAPTK )
      add_vidtk_tool( burnout_cli
        burnout_cli.cxx
        vidtk_video_io vidtk_object_detectors
        ${QT_LIBRARIES} kwiversys vital vital_apm
        vidtk_pipeline_framework vidtk_pipelines )
    else()
      add_vidtk_tool( burnout_cli
        burnout_cli.cxx
        vidtk_video_io vidtk_object_detectors
        ${QT_LIBRARIES}
        vidtk_pipeline_framework vidtk_pipelines )
    endif()
  endif()

  add_vidtk_tool( video_enhancement
    video_enhancement.cxx
    ${VIDTK_LIBRARIES} vil_algo vil vul )
endif()

if( NOT WIN32 )
  add_vidtk_tool( train_pixel_model
    train_pixel_model.cxx
    ${VIDTK_LIBRARIES} vil_algo vil vul )
endif()
