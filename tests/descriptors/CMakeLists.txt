# Tests that don't require any arguments at runtime
#
set( no_argument_test_sources
  test_bag_of_words_model.cxx
  test_descriptor_filter.cxx
  test_integral_hog_descriptor.cxx
  test_online_descriptor_generator.cxx
  test_online_track_type_classifier.cxx
  test_tot_collector_prob_product.cxx
  test_tot_collector_process.cxx
  test_tot_super_process.cxx
)

set( data_argument_test_sources
  )

if( VIDTK_ENABLE_OPENCV )
  set( data_argument_test_sources
    ${data_argument_test_sources}
    test_dpm_tot_generator.cxx
  )
endif()

create_test_sourcelist( test_sources
  test_driver.cxx

  # List of source files
  ${no_argument_test_sources}
  ${data_argument_test_sources}
)

add_executable( vidtk_test_descriptor
   ${test_sources}
)

target_link_libraries( vidtk_test_descriptor vidtk_descriptor vidtk_tracking testlib vul vpl )

# Create ADD_TEST lines for some tests automatically.  Tests taking
# special arguments will be specified manually later.

foreach( filename ${no_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_descriptor_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_descriptor> ${filebase} )
endforeach( filename )

foreach( filename ${data_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_descriptor_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_descriptor> ${filebase}
                    ${vidtk_tests_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR} )
endforeach( filename )
