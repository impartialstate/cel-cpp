# protobuf_generate function from standard CMake, but improved
function(protobuf_generate)
  set(options APPEND_PATH)
  set(oneValueArgs LANGUAGE OUT_VAR EXPORT_MACRO PROTOC_OUT_DIR)
  set(multiValueArgs PROTOS IMPORT_DIRS GENERATE_EXTENSIONS DEPENDENCIES)
  cmake_parse_arguments(protobuf_generate "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT protobuf_generate_PROTOS)
    message(SEND_ERROR "Error: protobuf_generate called without any proto files")
    return()
  endif()

  if(NOT protobuf_generate_OUT_VAR)
    message(SEND_ERROR "Error: protobuf_generate called without a variable name to store the generated files")
    return()
  endif()

  if(NOT protobuf_generate_LANGUAGE)
    set(protobuf_generate_LANGUAGE cpp)
  endif()
  string(TOLOWER ${protobuf_generate_LANGUAGE} protobuf_generate_LANGUAGE)

  if(NOT protobuf_generate_PROTOC_OUT_DIR)
    set(protobuf_generate_PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
  endif()

  if(NOT protobuf_generate_GENERATE_EXTENSIONS)
    if(protobuf_generate_LANGUAGE STREQUAL cpp)
      set(protobuf_generate_GENERATE_EXTENSIONS .pb.h .pb.cc)
    elseif(protobuf_generate_LANGUAGE STREQUAL python)
      set(protobuf_generate_GENERATE_EXTENSIONS _pb2.py)
    else()
      message(SEND_ERROR "Error: protobuf_generate given unknown Language ${LANGUAGE}, please provide GENERATE_EXTENSIONS")
      return()
    endif()
  endif()

  set(_import_arg)
  if(protobuf_generate_APPEND_PATH)
    foreach(_dir ${protobuf_generate_IMPORT_DIRS})
      list(APPEND _import_arg "-I${_dir}")
    endforeach()
  endif()

  foreach(_dir ${protobuf_generate_IMPORT_DIRS})
    get_filename_component(_abs_dir "${_dir}" ABSOLUTE)
    list(APPEND _import_arg "-I${_abs_dir}")
  endforeach()

  set(_generated_srcs_all)
  foreach(_proto ${protobuf_generate_PROTOS})
    get_filename_component(_abs_file "${_proto}" ABSOLUTE)
    get_filename_component(_basename "${_proto}" NAME_WE)
    
    # Find the shortest relative path from any IMPORT_DIR
    set(_best_import_dir)
    set(_shortest_path_len 999999)
    foreach(_dir ${protobuf_generate_IMPORT_DIRS})
      get_filename_component(_abs_dir "${_dir}" ABSOLUTE)
      file(RELATIVE_PATH _candidate_rel_file "${_abs_dir}" "${_abs_file}")
      if(NOT _candidate_rel_file MATCHES "^\\.\\." AND NOT IS_ABSOLUTE "${_candidate_rel_file}")
        string(LENGTH "${_candidate_rel_file}" _len)
        if(_len LESS _shortest_path_len)
          set(_shortest_path_len ${_len})
          set(_best_rel_file ${_candidate_rel_file})
          set(_best_import_dir ${_abs_dir})
        endif()
      endif()
    endforeach()

    if(NOT _best_rel_file)
      # Fallback to CMAKE_CURRENT_SOURCE_DIR
      file(RELATIVE_PATH _best_rel_file ${CMAKE_CURRENT_SOURCE_DIR} ${_abs_file})
      set(_best_import_dir ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    get_filename_component(_rel_dir ${_best_rel_file} DIRECTORY)

    set(_generated_srcs)
    foreach(_ext ${protobuf_generate_GENERATE_EXTENSIONS})
      list(APPEND _generated_srcs "${protobuf_generate_PROTOC_OUT_DIR}/${_rel_dir}/${_basename}${_ext}")
    endforeach()
    list(APPEND _generated_srcs_all ${_generated_srcs})

    add_custom_command(
      OUTPUT ${_generated_srcs}
      COMMAND  protobuf::protoc
      ARGS --${protobuf_generate_LANGUAGE}_out ${protobuf_generate_PROTOC_OUT_DIR} ${_import_arg} ${_best_rel_file}
      DEPENDS ${_abs_file} protobuf::protoc ${protobuf_generate_DEPENDENCIES}
      WORKING_DIRECTORY ${_best_import_dir}
      COMMENT "Running ${protobuf_generate_LANGUAGE} protocol buffer compiler on ${_proto}"
      VERBATIM )
  endforeach()

  set(${protobuf_generate_OUT_VAR} ${_generated_srcs_all} PARENT_SCOPE)
endfunction()
