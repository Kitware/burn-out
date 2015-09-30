#Tests that don't require any arguments at runtime
#
set( no_argument_test_sources
  test_da_so_tracker.cxx
  test_filter_image_objects_process.cxx
  test_track_initializer_process.cxx
  test_image_object_serialization.cxx
  test_image_difference_process.cxx
  test_da_tracker_process.cxx
  test_sg_gm_pixel_model.cxx
  test_pvo_class.cxx
  test_full_tracking_super_process.cxx
  test_homography_holder_process.cxx
  test_frame_objects_buffer_process.cxx
  test_track_attributes.cxx
  test_add_image_clip_to_image_object_process.cxx
  test_track_bbox_smoothing.cxx
  test_unique_track_id_process.cxx
  test_diff_super_process.cxx
)

# Tests that take the data directory as the only argument at runtime
#
set( data_argument_test_sources
  test_klt.cxx
  test_klt_tracker_process.cxx
  test_homography_process.cxx
  test_homography_reader_process.cxx
  test_mit_reader.cxx
  test_multi_feature.cxx
  test_viper_reader.cxx
  test_kw18_reader.cxx
  test_vsl_reader.cxx
  test_track_reader_process.cxx
  test_track.cxx
  test_metadata_mask_pipeline.cxx
  test_shot_stitching_algo.cxx
  test_shot_stitching_process.cxx
  test_transform_tracks_process.cxx
  test_shot_break_monitor.cxx
  test_connected_component_process.cxx
  test_reconcile_tracks_process.cxx
)

# Tests that take the EC data directory as the only argument at runtime
#
set( ec_data_argument_test_sources
  test_world_coord_super_process.cxx
)

create_test_sourcelist( test_sources
  test_driver.cxx

  # List of source files
  ${no_argument_test_sources}
  ${data_argument_test_sources}
  ${ec_data_argument_test_sources}
)

add_executable( vidtk_test_tracking
   ${test_sources}
)
target_link_libraries( vidtk_test_tracking vidtk_klt vidtk_tracking vidtk_pipeline testlib vul vpl )

add_executable( track_bbox_kw18 track_bbox_kw18.cxx)
target_link_libraries( track_bbox_kw18 vidtk_tracking vidtk_pipeline testlib vul vpl )

# Create ADD_TEST lines for some tests automatically.  Tests taking
# special arguments will be specified manually later.

foreach( filename ${no_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_tracking_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_tracking> ${filebase} )
endforeach( filename )

foreach( filename ${data_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_tracking_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_tracking> ${filebase}
                    ${vidtk_tests_SOURCE_DIR}/data )
endforeach( filename )

if (ENABLE_EXPORT_CONTROLLED_TESTS)
  foreach( filename ${ec_data_argument_test_sources} )
    string( REPLACE ".cxx" "" filebase ${filename} )
    string( REPLACE "test_" "" testname ${filebase} )
    add_test( NAME vidtk_tracking_${testname}
              COMMAND $<TARGET_FILE:vidtk_test_tracking> ${filebase}
                      ${CMAKE_SOURCE_DIR}/ec/vidtk/tests/data )
  endforeach( filename )
endif ()

#############################################################################
#                        Detect and track test cases                        #
#############################################################################

option( ENABLE_DETECT_AND_TRACK_TESTS
        "Enable Detect_and_track tests" ON )

find_package( PythonInterp )

if (ENABLE_DETECT_AND_TRACK_TESTS AND PYTHONINTERP_FOUND)
  macro( add_detect_and_track_test name test_filename config trk_config video_path timeout )
    configure_file( ${DET_AND_TRK_CONFIG_IN_DIR}/${config}
                    ${DET_AND_TRK_CONFIG_DIR}/${config}
                    @ONLY )
    configure_file( ${DET_AND_TRK_CONFIG_IN_DIR}/${trk_config}
                    ${DET_AND_TRK_CONFIG_DIR}/${trk_config}
                    @ONLY )
    set( runtime_output
      ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )
    if (WIN32)
      set( runtime_output
        "${runtime_output}/$<CONFIGURATION>")
    endif (WIN32)
    midas_add_test( NAME ${name}
                    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${test_filename}
                            ${runtime_output}
                            ${DET_AND_TRK_CONFIG_DIR}
                            ${video_path}
                            ${config}
                            ${trk_config}
                            ${CMAKE_BINARY_DIR}/tests/detect_and_track/${name}
                            ${DET_AND_TRACK_RESULTS_ROOT}/${name}
                            ${ARGN} )
    set_tests_properties( ${name}
      PROPERTIES
        TIMEOUT ${timeout})
  endmacro()

  set( DET_AND_TRK_CONFIG_IN_DIR
       ${CMAKE_SOURCE_DIR}/tests/data/config_files )
  set( DET_AND_TRK_CONFIG_DIR
       ${CMAKE_BINARY_DIR}/tests/data/config_files )

  set( DAT_TESTS_PUB_RESULTS_ROOT ""
    CACHE PATH "Where to store public results for detect_and_track tests" )

  set( DET_AND_TRACK_RESULTS_ROOT
    "${DAT_TESTS_PUB_RESULTS_ROOT}")

  # Example:
  #
  #   add_detect_and_track_test( dat_aphill_amhi_async
  #     dat_aphill_amhi_async.py
  #     dat_aphill_amhi_async.dat_conf
  #     dat_aphill_amhi_async.conf
  #     aphill_video1.mp4
  #     300
  #     more_args.aaaaaaarg ) # like this, we can add more arguments after the
  #                           # required ones.
  #
  # Runs detect_and_track with the given configuration over the
  # aphill_video1.mp4 video with a timeout of 5 minutes. Further arguments may
  # be given to download additional data files. Configurations will have
  # "@MIDAS_DATA_DIR@" in the tracker and tool configuration file replaced by
  # the data directory.

  #Non-MIDAS Tests
  add_detect_and_track_test( dat_tracking_sp_writer_interference
    test_detect_and_track.py
    dat_tracking_sp_writer_interference.conf
    dat_tracking_sp_writer_interference-trkr.conf
    "none"
    300 ) # 5 minute time-out

  add_detect_and_track_test( dat_top_level_output_pad
    test_detect_and_track.py
    dat_top_level_output_pad.conf
    dat_top_level_output_pad-trkr.conf
    "none"
    300 ) # 5 minute time-out

  #MIDAS Tests

  # Include export-controlled tests if found
  if( ENABLE_EXPORT_CONTROLLED_TESTS )
    #set( MIDAS_REST_URL "https://USER@videonas2/midas/api/rest" )
    #set( MIDAS_REST_URL "https://USER:PASS@videonas2/midas/api/rest" )
    set( MIDAS_KEY_DIR "${MIDAS_KEY_DIR}/ec" )
    set( MIDAS_DATA_DIR "${MIDAS_DATA_DIR}/ec" )

    set( DAT_TESTS_EC_RESULTS_ROOT ""
      CACHE PATH "Where to store export-controlled results for detect_and_track tests" )

    set( DET_AND_TRACK_RESULTS_ROOT
      "${DAT_TESTS_EC_RESULTS_ROOT}")

    set( DET_AND_TRK_CONFIG_IN_DIR
         ${DET_AND_TRK_CONFIG_IN_DIR}/ec )
    set( DET_AND_TRK_CONFIG_DIR
         ${DET_AND_TRK_CONFIG_DIR}/ec )

  endif()
endif()
