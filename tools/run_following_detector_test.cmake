configure_file( ${DATA_PATH}/following_test.config
                ${CMAKE_CURRENT_BINARY_DIR}/following_test.config )

# remove any previous output file
execute_process( COMMAND ${CMAKE_COMMAND} -E remove  
                 ${CMAKE_CURRENT_BINARY_DIR}/following_events.txt)

execute_process(
  COMMAND ${DETECT_EVENTS_EXECUTABLE}
          -c ${CMAKE_CURRENT_BINARY_DIR}/following_test.config
  OUTPUT_FILE  ${CMAKE_CURRENT_BINARY_DIR}/detect_following_events_output.txt
  ERROR_FILE  ${CMAKE_CURRENT_BINARY_DIR}/detect_following_events_error.txt	
  ERROR_VARIABLE test_error
  RESULT_VARIABLE test_result)

if (NOT ${test_result} EQUAL 0)
   message(FATAL_ERROR "detect_events returned non-zero exit code")
endif ()

