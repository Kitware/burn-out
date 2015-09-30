configure_file( ${DATA_PATH}/apply_saliency_event_norm_time_diff.config
                ${CMAKE_CURRENT_BINARY_DIR}/apply_saliency_event_norm_time_diff.config )

# remove any previous output file
execute_process( COMMAND ${CMAKE_COMMAND} -E remove
                 ${CMAKE_CURRENT_BINARY_DIR}/saliency_event_norm_time_diff_ltf_activities.txt)

execute_process(
  COMMAND ${APPLY_ACTIVITY_DETECTOR_EXECUTABLE}
          -c ${CMAKE_CURRENT_BINARY_DIR}/apply_saliency_event_norm_time_diff.config
  OUTPUT_FILE  ${CMAKE_CURRENT_BINARY_DIR}/detect_apply_saliency_event_norm_time_diff_output.txt
  ERROR_FILE  ${CMAKE_CURRENT_BINARY_DIR}/detect_apply_saliency_event_norm_time_diff_error.txt
  ERROR_VARIABLE test_error
  RESULT_VARIABLE test_result)

if (NOT ${test_result} EQUAL 0)
   message(FATAL_ERROR "detect_activities returned non-zero exit code")
endif ()
