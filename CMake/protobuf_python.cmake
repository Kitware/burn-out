# ----------- PYTHON protobuf support ---------------


function(PROTOBUF_GENERATE_PYTHON SRCS)
  if(NOT ARGN)
    message(SEND_ERROR "Error: PROTOBUF_GENERATE_PYTHON() called without any proto files")
    return()
  endif()

  if (NOT PROTOBUF_PYTHON_OUTPUT)
    set(PROTOBUF_PYTHON_OUTPUT ${CMAKE_CURRENT_BINARY_DIR})
  endif()

  if(PROTOBUF_GENERATE_PYTHON_APPEND_PATH)
    # Create an include path for each file specified
    foreach(FIL ${ARGN})
      get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
      get_filename_component(ABS_PATH ${ABS_FIL} PATH)
      list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
      if(${_contains_already} EQUAL -1)
          list(APPEND _protobuf_include_path -I ${ABS_PATH})
      endif()
    endforeach()
  else()
    set(_protobuf_include_path -I ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  if(DEFINED PROTOBUF_IMPORT_DIRS)
    foreach(DIR ${PROTOBUF_IMPORT_DIRS})
      get_filename_component(ABS_PATH ${DIR} ABSOLUTE)
      list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
      if(${_contains_already} EQUAL -1)
          list(APPEND _protobuf_include_path -I ${ABS_PATH})
      endif()
    endforeach()
  endif()

  set(${SRCS})
  foreach(FIL ${ARGN})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)

    # put output in "python" subdirectory
    set(_output_stem "${PROTOBUF_PYTHON_OUTPUT}/${FIL_WE}")

    list(APPEND ${SRCS} "${_output_stem}_pb2.py")

    add_custom_command(
      OUTPUT "${_output_stem}_pb2.py"
      COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
      ARGS --python_out  ${PROTOBUF_PYTHON_OUTPUT}  ${_protobuf_include_path} ${ABS_FIL}
      DEPENDS ${ABS_FIL}
      COMMENT "Running Python protocol buffer compiler on ${FIL}"
      VERBATIM)
  endforeach()

  set_source_files_properties(${${SRCS}} PROPERTIES GENERATED TRUE)
  set(${SRCS} ${${SRCS}} PARENT_SCOPE)
endfunction()
