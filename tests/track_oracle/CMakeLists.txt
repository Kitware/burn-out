# Tests that take the data directory as the only argument at runtime
#

set( data_argument_test_sources
     test_track_oracle_thread_safety.cxx
     test_score_hadwav.cxx
     test_score_aipr.cxx
     test_score_frames_aipr.cxx
     test_bounding_box.cxx
     test_align_frames.cxx
     test_domains.cxx
     test_aries_interface.cxx
     test_min_gt_ct_pcent.cxx
     test_track_xgtf.cxx
     test_track_kwxml.cxx
     test_track_scorable_mgrs.cxx
     test_track_synthesizer.cxx
     test_time_window_filter.cxx
     test_continuity_purity.cxx
     test_track_oracle_geo_aoi.cxx
     test_track_loader.cxx
     test_csv_tokenizer.cxx
     test_clone_fields.cxx
     test_data_terms.cxx
     test_aoi_utils.cxx
     test_xml_tokenizer.cxx
)

create_test_sourcelist( test_sources
                       test_driver.cxx
                       ${data_argument_test_sources}
                      )

add_executable( test_score_tracks
                ${test_sources}
)

target_link_libraries( test_score_tracks 
                         score_tracks_loader
                         score_tracks_hadwav
                         score_tracks_aipr
                         score_frames_aipr
                         track_oracle
                         data_terms
                         track_oracle_file_formats
                         vidtk_logger
                         testlib
                         scoring_aries_interface
                         track_xgtf
                         track_oracle_file_formats
                         track_scorable_mgrs
                         track_synthesizer
                         timestamp_utilities
                         track_oracle_aoi_utils
                         track_oracle_tokenizers
                         ${VIDTK_LIBRARIES})

add_test( NAME test_track_oracle_thread_safety
          COMMAND $<TARGET_FILE:test_score_tracks> test_track_oracle_thread_safety
          "${vidtk_tests_SOURCE_DIR}/data" )

add_test( NAME test_score_hadwav
          COMMAND $<TARGET_FILE:test_score_tracks> test_score_hadwav
                  "${vidtk_tests_SOURCE_DIR}/data" )

add_test( NAME test_score_aipr
          COMMAND $<TARGET_FILE:test_score_tracks> test_score_aipr
                  "${vidtk_tests_SOURCE_DIR}/data" )

add_test( NAME test_score_frames_aipr
          COMMAND $<TARGET_FILE:test_score_tracks> test_score_frames_aipr
                  "${vidtk_tests_SOURCE_DIR}/data" )

add_test( NAME test_bounding_box
          COMMAND $<TARGET_FILE:test_score_tracks> test_bounding_box)

add_test( NAME test_align_frames
          COMMAND $<TARGET_FILE:test_score_tracks> test_align_frames
                  "${vidtk_tests_SOURCE_DIR}/data" )

add_test( NAME test_domains
          COMMAND $<TARGET_FILE:test_score_tracks> test_domains)

add_test( NAME test_track_kwxml
          COMMAND $<TARGET_FILE:test_score_tracks> test_track_kwxml
                  "${vidtk_tests_SOURCE_DIR}/data" )

add_test( NAME test_aries_interface
          COMMAND $<TARGET_FILE:test_score_tracks> test_aries_interface )

add_test( NAME test_min_gt_ct_pcent
          COMMAND $<TARGET_FILE:test_score_tracks> test_min_gt_ct_pcent )

add_test( NAME test_track_xgtf
          COMMAND $<TARGET_FILE:test_score_tracks> test_track_xgtf
          "${vidtk_tests_SOURCE_DIR}/data"
)
add_test( NAME test_track_scorable_mgrs
          COMMAND $<TARGET_FILE:test_score_tracks> test_track_scorable_mgrs
          "${vidtk_tests_SOURCE_DIR}/data" )

add_test( NAME test_track_synthesizer
          COMMAND $<TARGET_FILE:test_score_tracks> test_track_synthesizer )

add_test( NAME test_time_window_filter
          COMMAND $<TARGET_FILE:test_score_tracks> test_time_window_filter )

add_test( NAME test_continuity_purity
          COMMAND $<TARGET_FILE:test_score_tracks> test_continuity_purity )

add_test( NAME test_track_oracle_geo_aoi
          COMMAND $<TARGET_FILE:test_score_tracks> test_track_oracle_geo_aoi )

add_test( NAME test_track_loader
          COMMAND $<TARGET_FILE:test_score_tracks> test_track_loader
                  "${vidtk_tests_SOURCE_DIR}/data" )

add_test( NAME test_csv_tokenizer
          COMMAND $<TARGET_FILE:test_score_tracks> test_csv_tokenizer )

add_test( NAME test_clone_fields
          COMMAND $<TARGET_FILE:test_score_tracks> test_clone_fields )

add_test( NAME test_data_terms
          COMMAND $<TARGET_FILE:test_score_tracks> test_data_terms )

add_test( NAME test_aoi_utils
          COMMAND $<TARGET_FILE:test_score_tracks> test_aoi_utils )

add_test( NAME test_xml_tokenizer
          COMMAND $<TARGET_FILE:test_score_tracks> test_xml_tokenizer
          "${vidtk_tests_SOURCE_DIR}/data" )
