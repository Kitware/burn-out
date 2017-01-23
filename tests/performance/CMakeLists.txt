set( detect_and_track_performance_ap_hill_config
  ${CMAKE_SOURCE_DIR}/tools/config_files/dat_aphill_timing.conf )

option( VIDTK_PERFORMANCE_REALLY_LONG_TESTS "Enable long performance tests" OFF )

if( VIDTK_PERFORMANCE_REALLY_LONG_TESTS )
midas_add_test( NAME detect_and_track_performance_ap_hill_1
    COMMAND $<TARGET_FILE:detect_and_track>
            -c ${detect_and_track_performance_ap_hill_config}
            detect_and_track:src:vidl_ffmpeg:filename=MIDAS{sample_aphill.mp4.md5} )
endif()

midas_add_test( NAME detect_and_track_performance_ap_hill_2
    COMMAND $<TARGET_FILE:detect_and_track>
            -c ${detect_and_track_performance_ap_hill_config}
            detect_and_track:src:vidl_ffmpeg:filename=MIDAS{sample_aphill_short.mp4.md5} )

set_tests_properties( detect_and_track_performance_ap_hill_2
    PROPERTIES
        TIMEOUT 3600)

add_executable( test_gauss_filter_time
  test_gauss_filter_2d_timing.cxx
  )

target_link_libraries( test_gauss_filter_time
  vidtk_video_io vidtk_video_transforms vcl vsl vil vil_io vil_algo
   )
