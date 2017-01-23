# Tests that don't require any arguments at runtime
#
set( no_argument_test_sources
  test_blob_pixel_feature_extraction.cxx
  test_diff_super_process.cxx
  test_moving_training_data_container.cxx
  test_pixel_feature_extraction_super_process.cxx
  test_sg_gm_pixel_model.cxx
  test_three_frame_differencing.cxx
)

set( data_argument_test_sources
  test_connected_component_process.cxx
  test_filter_image_objects_process.cxx
  test_salient_region_classifier_process.cxx
  test_text_parser.cxx
)

if( VIDTK_ENABLE_OPENCV )
  set( data_argument_test_sources
    ${data_argument_test_sources}
    test_sthog_detector_process.cxx
    )
endif()

create_test_sourcelist( test_sources
  test_driver.cxx

  # List of source files
  ${no_argument_test_sources}
  ${data_argument_test_sources}
)

add_executable( vidtk_test_object_detectors
   ${test_sources}
)

if( VIDTK_ENABLE_OPENCV )
  target_link_libraries( vidtk_test_object_detectors ${OpenCV_LIBS} vidtk_classifiers )
endif()

target_link_libraries( vidtk_test_object_detectors vidtk_object_detectors vidtk_video_io testlib vul vpl )

# Create ADD_TEST lines for some tests automatically.  Tests taking
# special arguments will be specified manually later.

foreach( filename ${no_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_object_detectors_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_object_detectors> ${filebase} )
endforeach( filename )

foreach( filename ${data_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_object_detectors_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_object_detectors> ${filebase}
                    ${vidtk_tests_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR} )
endforeach( filename )
