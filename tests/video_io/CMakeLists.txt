# Tests that don't require any arguments at runtime
#
set( no_argument_test_sources
  test_frame_process_sp_facade.cxx
)

# Tests that take the data directory as the only argument at runtime
#
set( data_argument_test_sources
  test_image_list_frame_process.cxx
  test_vidl_ffmpeg_frame_process.cxx
  test_image_list_frame_metadata_process.cxx
  test_frame_metadata_decoder_process.cxx
  filename_frame_metadata_process.cxx
  test_frame_metadata_super_process.cxx
  test_mask_reader_process.cxx
)

create_test_sourcelist( test_sources
  test_driver.cxx
  test_vidl_ffmpeg_frame_process_klv.cxx
  test_vidl_ffmpeg_frame_process_misp.cxx
  # List of source files
  ${no_argument_test_sources}
  ${data_argument_test_sources}
)

configure_file( ${vidtk_tests_SOURCE_DIR}/data/smallframe_file.txt
                ${CMAKE_CURRENT_BINARY_DIR}/smallframe_file.txt )
configure_file( ${vidtk_tests_SOURCE_DIR}/data/smallframe_file_meta.txt
                ${CMAKE_CURRENT_BINARY_DIR}/smallframe_file_meta.txt )
configure_file( ${vidtk_tests_SOURCE_DIR}/data/smallframe_file_meta_missmatch.txt
                ${CMAKE_CURRENT_BINARY_DIR}/smallframe_file_meta_missmatch.txt )
configure_file( ${vidtk_tests_SOURCE_DIR}/data/red_river_list.txt
                ${CMAKE_CURRENT_BINARY_DIR}/red_river_list.txt )

add_executable( vidtk_test_video_io
   ${test_sources}
)

target_link_libraries( vidtk_test_video_io vidtk_video_io vgl_algo testlib vnl vul vpl )

# Create ADD_TEST lines for some tests automatically.  Tests taking
# special arguments will be specified manually later.

foreach( filename ${no_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_video_io_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_video_io> ${filebase} )
endforeach( filename )

foreach( filename ${data_argument_test_sources} )
  string( REPLACE ".cxx" "" filebase ${filename} )
  string( REPLACE "test_" "" testname ${filebase} )
  add_test( NAME vidtk_video_io_${testname}
            COMMAND $<TARGET_FILE:vidtk_test_video_io> ${filebase}
                    ${vidtk_tests_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR} )
endforeach( filename )

midas_add_test(NAME vidtk_video_io_vidl_ffmpeg_frame_process_misp
               COMMAND $<TARGET_FILE:vidtk_test_video_io> test_vidl_ffmpeg_frame_process_misp
               MIDAS{09152008flight2tape3_1.mpg.md5} )

if( ENABLE_EXPORT_CONTROLLED_TESTS )
  include( ${vidtk_source_dir}/ec/dashboard_tests/data/MidasECAuth.cmake )
  set(MIDAS_DEFAULT_API_KEY "${MIDAS_TRACKER_EC_DEFAULT_API_KEY}")
  set(MIDAS_USER "${MIDAS_TRACKER_EC_LOGIN}")

  set( MIDAS_REST_URL "https://ecmidas/midas/api/rest" )
  set( MIDAS_KEY_DIR "${CMAKE_SOURCE_DIR}/ec/dashboard_tests/data/midas" )
  set( MIDAS_DATA_DIR "${CMAKE_BINARY_DIR}/tests/data/ec/midas" )

  midas_add_test( NAME vidtk_video_io_vidl_ffmpeg_frame_process_klv
                  COMMAND $<TARGET_FILE:vidtk_test_video_io> test_vidl_ffmpeg_frame_process_klv
                  MIDAS{U-ITAR_20090319_1115_I2_10_stamped.mpg.md5}
                  ${vidtk_source_dir}/ec/dashboard_tests/data/U-ITAR_20090319_1115_I2_10_stamped-metatest.vidtk_meta )
endif()
