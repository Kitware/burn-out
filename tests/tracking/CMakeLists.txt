#Tests that don't require any arguments at runtime
#
set( no_argument_test_sources
  test_stabilization_super_process.cxx
)

# Tests that take the data directory as the only argument at runtime
#
set( data_argument_test_sources
  test_klt.cxx
  test_homography_process.cxx
  test_klt_shot_stitching_algo.cxx
  test_shot_stitching_process.cxx
  test_refine_homography.cxx
)

if( VIDTK_ENABLE_MAPTK )
  list(APPEND data_argument_test_sources
    test_maptk_shot_stitching_algo.cxx
    )
endif()

create_test_sourcelist( test_sources
  test_driver.cxx

  # List of source files
  ${no_argument_test_sources}
  ${data_argument_test_sources}
)

add_executable( vidtk_test_tracking
   ${test_sources}
)
target_link_libraries( vidtk_test_tracking vidtk_klt vidtk_tracking vidtk_video_io vidtk_pipeline_framework testlib vul vpl )

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
