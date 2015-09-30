project( vidtk_apps )

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

find_package( VXL REQUIRED )
include( ${VXL_CMAKE_DIR}/UseVXL.cmake )
include(GitVersionTest)
check_reachable_git_hash(VXL ${VXL_CMAKE_DIR} "69ee84425ac42d02b7c8598f91fa8cd7d485c458" QUIET)

find_package( TinyXML 2.6 REQUIRED )
include_directories( ${TinyXML_INCLUDE_DIR} )

set( BOOST_ROOT "" CACHE PATH "The root path to Boost" )
find_package( Boost REQUIRED )
add_definitions( -DBOOST_ALL_NO_LIB )
include_directories( ${Boost_INCLUDE_DIRS} )

if( VIDTK_ENABLE_OPENCV )
  find_package( OpenCV REQUIRED )
endif()

include_directories( ${vidtk_SOURCE_DIR} ${vidtk_BINARY_DIR} )


include_directories( ${VXL_VCL_INCLUDE_DIR} ${VXL_CORE_INCLUDE_DIR} )
include_directories( ${VXL_BRL_INCLUDE_DIR} )
include_directories( ${VXL_GEL_INCLUDE_DIR}/mrc )
include_directories( ${VXL_RPL_INCLUDE_DIR} )

if( VXL_VGUI_FOUND )
  add_executable( mod mod.cxx)
  target_link_libraries( mod  ${VIDTK_LIBRARIES} vgui vil_algo vil vul )

  add_executable( track_annotate_video track_annotate_video.cxx)
  target_link_libraries( track_annotate_video ${VIDTK_LIBRARIES} vgui vnl vul )

  add_executable( track_da_pipeline track_da_pipeline.cxx)
  target_link_libraries( track_da_pipeline  ${VIDTK_LIBRARIES} vgui vil_algo vil vul )

#  add_executable( manual_pmatrix manual_pmatrix.cxx)
#  target_link_libraries( manual_pmatrix ${VIDTK_LIBRARIES} vgui vgl_algo vgl vnl vil vul )
endif( VXL_VGUI_FOUND )


# =================================================================
# Detect and Track Tools
# =================================================================

if( NOT VXL_VGUI_FOUND )

  add_vidtk_tool( detect_and_track
    detect_and_track.cxx
    "-DNOGUI"
    ${VIDTK_LIBRARIES} vil_algo vil vul
    )

  add_vidtk_tool( detect_and_track_async
    detect_and_track_async.cxx
    "-DNOGUI"
    ${VIDTK_LIBRARIES} vil_algo vil vul
    )

else( )

  add_vidtk_tool( detect_and_track
    detect_and_track.cxx
    ${VIDTK_LIBRARIES} vil_algo vil vul vgui
    )

  add_vidtk_tool( detect_and_track_async
    detect_and_track_async.cxx
    "-DNOGUI"
    ${VIDTK_LIBRARIES} vil_algo vil vul
    )

endif() # if NOT VXL_VGUI_FOUND



#add_executable( detect_and_track detect_and_track.cxx)
#if( NOT VXL_VGUI_FOUND )
#  set_target_properties( detect_and_track
#                         PROPERTIES COMPILE_FLAGS "-DNOGUI" )
#else( NOT VXL_VGUI_FOUND )
#  target_link_libraries( detect_and_track  vgui )
#endif( NOT VXL_VGUI_FOUND )
#target_link_libraries( detect_and_track  ${VIDTK_LIBRARIES} vil_algo vil vul )
#
#add_executable( detect_and_track_async detect_and_track_async.cxx)
#set_target_properties( detect_and_track_async
#                       PROPERTIES COMPILE_FLAGS "-DNOGUI" )
#target_link_libraries( detect_and_track_async  ${VIDTK_LIBRARIES} vil_algo vil vul )
#
add_executable( video_transcoder video_transcoder.cxx )
set_target_properties( video_transcoder
                       PROPERTIES COMPILE_FLAGS "-DNOGUI" )

target_link_libraries( video_transcoder  ${VIDTK_LIBRARIES} vil_algo vil vul )

if( VIDTK_ENABLE_OPENCV AND OpenCV_FOUND )
  add_executable( remove_metadata_burnin remove_metadata_burnin.cxx )
  target_link_libraries( remove_metadata_burnin vidtk_video vidtk_pipeline )
endif()

add_executable( timestamp_test timestamp_test.cxx)
target_link_libraries( timestamp_test ${VIDTK_LIBRARIES} vil )

add_executable( oc_test oc_test.cxx)
target_link_libraries( oc_test ${VIDTK_LIBRARIES} vul )

add_executable( mod_annotate_video mod_annotate_video.cxx)
target_link_libraries( mod_annotate_video ${VIDTK_LIBRARIES} vil vul )

add_executable( extract_video_frames extract_video_frames.cxx)
target_link_libraries( extract_video_frames ${VIDTK_LIBRARIES} vil )

add_executable( score_homographies score_homographies.cxx  )
target_link_libraries( score_homographies ${VIDTK_LIBRARIES} )

#add_executable( score_frames score_frames.cxx  )
#target_link_libraries( score_frames ${VIDTK_LIBRARIES} score_frames_aipr)

add_executable( convert_track_format convert_track_format.cxx)
target_link_libraries( convert_track_format ${VIDTK_LIBRARIES} )

#transform_track_location
add_executable( transform_track_location transform_track_location.cxx)
target_link_libraries( transform_track_location ${VIDTK_LIBRARIES} )

add_executable( interpolate_gps interpolate_gps.cxx)
target_link_libraries( interpolate_gps ${VIDTK_LIBRARIES} )

# vsl_track_to_pvo_text.cxx
add_executable( vsl_track_to_pvo_text vsl_track_to_pvo_text.cxx)
target_link_libraries( vsl_track_to_pvo_text ${VIDTK_LIBRARIES} )

# pvo_trac_classification_roc_input_tile_generator.cxx
add_executable( pvo_trac_classification_threshold_sweep pvo_trac_classification_threshold_sweep.cxx)
target_link_libraries( pvo_trac_classification_threshold_sweep ${VIDTK_LIBRARIES} )

add_executable( track_statistics track_statistics.cxx)
target_link_libraries( track_statistics ${VIDTK_LIBRARIES} )

#add_executable( test_init_change test_init_change.cxx)
#target_link_libraries( test_init_change ${VIDTK_LIBRARIES} )

# vsl_track_to_adaboost_descriptor.cxx
#add_executable( vsl_track_to_adaboost_descriptor vsl_track_to_adaboost_descriptor.cxx)
#target_link_libraries( vsl_track_to_adaboost_descriptor ${VIDTK_LIBRARIES} )

# create_positive_and_negative_set.cxx
#add_executable( create_positive_and_negative_set create_positive_and_negative_set.cxx)
#target_link_libraries( create_positive_and_negative_set ${VIDTK_LIBRARIES} )

# multi_dataset_training_test.cxx
#add_executable( multi_dataset_training_test multi_dataset_training_test.cxx)
#target_link_libraries( multi_dataset_training_test ${VIDTK_LIBRARIES} )

# test_image_sequence_accessor.cxx
add_executable( test_image_sequence_accessor test_image_sequence_accessor.cxx)
target_link_libraries( test_image_sequence_accessor ${VIDTK_LIBRARIES} )

# circular_features_test.cxx
#add_executable( circular_features_test circular_features_test.cxx)
#target_link_libraries( circular_features_test ${VIDTK_LIBRARIES} )

# =================================================================
# KLT tracks
# =================================================================
if( VXL_VGUI_FOUND )
  add_executable( klt_tracks klt_tracks.cxx)
  target_link_libraries( klt_tracks ${VIDTK_LIBRARIES} vgui vil_algo vil vul )
endif( VXL_VGUI_FOUND )

add_executable( klt_tracks_nogui klt_tracks.cxx)
set_target_properties( klt_tracks_nogui
                       PROPERTIES COMPILE_FLAGS "-DNOGUI" )
target_link_libraries( klt_tracks_nogui ${VIDTK_LIBRARIES} vil_algo vil vul )


# =================================================================
# Stabilize video
# =================================================================
if( VXL_VGUI_FOUND )
  add_executable( stabilize_video stabilize_video.cxx)
  target_link_libraries( stabilize_video  ${VIDTK_LIBRARIES} vgui vil_algo vil vul )
  add_executable( stabilize_video_old stabilize_video_old.cxx)
  target_link_libraries( stabilize_video_old  ${VIDTK_LIBRARIES} vgui vil_algo vil vul )
endif( VXL_VGUI_FOUND )

add_executable( stabilize_video_nogui stabilize_video.cxx)
set_target_properties( stabilize_video_nogui
                       PROPERTIES COMPILE_FLAGS "-DNOGUI" )
target_link_libraries( stabilize_video_nogui ${VIDTK_LIBRARIES} vil_algo vil vul )


# =================================================================
# warp video
# =================================================================
if( VXL_VGUI_FOUND )
  add_executable( warp_video warp_video.cxx)
  target_link_libraries( warp_video  ${VIDTK_LIBRARIES} vgui vgl_algo vil_algo vil vul )
endif( VXL_VGUI_FOUND )

add_executable( warp_video_nogui warp_video.cxx)
set_target_properties( warp_video_nogui
                       PROPERTIES COMPILE_FLAGS "-DNOGUI" )

target_link_libraries( warp_video_nogui ${VIDTK_LIBRARIES} vgl_algo vil_algo vil vul )

add_executable( warp_video_16bit_nogui warp_video.cxx)
set_target_properties( warp_video_16bit_nogui
                       PROPERTIES COMPILE_FLAGS "-DNOGUI -DPIXELTYPE=vxl_uint_16" )
target_link_libraries( warp_video_16bit_nogui ${VIDTK_LIBRARIES} vgl_algo vil_algo vil vul )


# =================================================================
# Estimate a sequence of fundamental matrices from tracked points
# =================================================================
add_executable( estimate_f_matrices estimate_f_matrices.cxx)
target_link_libraries( estimate_f_matrices ${VIDTK_LIBRARIES} vpgl_algo vpgl vul )


# =================================================================
# Convert between Geographic coordinates and local cartesian coordinates
# =================================================================
add_executable( convert_geo_coords convert_geo_coords.cxx)
target_link_libraries( convert_geo_coords ${VIDTK_LIBRARIES} vpgl )


# =================================================================
# zoom video
# =================================================================
add_executable( zoom_video zoom_video.cxx)
target_link_libraries( zoom_video ${VIDTK_LIBRARIES} vil vul )

# =================================================================
# train an adaboost track classifier
# =================================================================
#add_executable( train_adaboost_classifer train_adaboost_classifer.cxx)
#target_link_libraries( train_adaboost_classifer ${VIDTK_LIBRARIES} vul )

# =================================================================
# apply an adaboost track classifier
# =================================================================
#add_executable( classify_and_filter_tracks classify_and_filter_tracks.cxx)
#target_link_libraries( classify_and_filter_tracks ${VIDTK_LIBRARIES} vul )

# test_classification.cxx
#add_executable( test_classification test_classification.cxx)
#target_link_libraries( test_classification ${VIDTK_LIBRARIES} vul )

# =================================================================
# scoring code for the ICCV09 functional modeling paper
# =================================================================
add_executable( scoreICCV09 scoreICCV09.cxx )
target_link_libraries( scoreICCV09 vul vil )

# =================================================================
# Track Linker
# =================================================================
add_executable( link_tracks link_tracks.cxx)
target_link_libraries( link_tracks vil vul ${VIDTK_LIBRARIES} )

# =================================================================
# train simple spatial events normalcy model
# =================================================================

# does the generated normalcy model match the expected output?
add_test( compare_event_normalcy
          ${CMAKE_COMMAND} -E compare_files
          normalcy_histogram.txt
          ${vidtk_all_SOURCE_DIR}/tests/data/reference_normalcy_histogram.dat)
set_tests_properties (compare_event_normalcy PROPERTIES DEPENDS train_event_normalcy)


# does the generated normalcy model match the expected output?
add_test( compare_apply_saliency_time_link_prob
          ${CMAKE_COMMAND} -E compare_files
          saliency_time_link_ltf_activities.txt
          ${vidtk_all_SOURCE_DIR}/tests/data/reference_saliency_time_link_ltf_activities.dat)
set_tests_properties (compare_apply_saliency_time_link_prob PROPERTIES DEPENDS test_apply_saliency_time_link_prob)


# does the generated normalcy model match the expected output?
add_test( compare_apply_saliency_event_norm_time_diff
          ${CMAKE_COMMAND} -E compare_files
          saliency_event_norm_time_diff_ltf_activities.txt
          ${vidtk_all_SOURCE_DIR}/tests/data/reference_saliency_event_norm_time_diff_ltf_activities.dat)
set_tests_properties (compare_apply_saliency_event_norm_time_diff PROPERTIES DEPENDS test_apply_saliency_event_norm_time_diff)

