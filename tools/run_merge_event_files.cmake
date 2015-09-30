# remove any previous output file
execute_process( COMMAND ${CMAKE_COMMAND} -E remove
                 ${CMAKE_CURRENT_BINARY_DIR}/meeting_plus_aimless.event.txt)

execute_process(
  COMMAND ${MERGE_EVENTS_EXE}
          --file-a ${DATA_PATH}/reference_meeting_events.dat
          --file-b ${DATA_PATH}/reference_aimless_driving_events.dat
          --output ${CMAKE_CURRENT_BINARY_DIR}/meeting_plus_aimless.event.txt
  OUTPUT_FILE  ${CMAKE_CURRENT_BINARY_DIR}/merge_events_meeting_aimless_output.txt
  ERROR_FILE  ${CMAKE_CURRENT_BINARY_DIR}/merge_events_meeting_aimless_error.txt
  ERROR_VARIABLE test_error
  RESULT_VARIABLE test_result)

if (NOT ${test_result} EQUAL 0)
   message(FATAL_ERROR "detect_events returned non-zero exit code")
endif ()
