cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_SKIP_INSTALL_ALL TRUE)
project(cel-cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# --- Options ---
option(BUILD_TESTING "Build the tests" OFF)

if(BUILD_TESTING)
  include(CTest)
  enable_testing()
endif()

# --- Googletest ---
if(BUILD_TESTING)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0
  )
  # For Windows: GTest might need these
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

# --- Abseil ---
FetchContent_Declare(
  absl
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG        20260107.1
)
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
set(ABSL_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(ABSL_BUILD_TESTING OFF CACHE BOOL "" FORCE)
if(BUILD_TESTING)
  set(ABSL_BUILD_TEST_HELPERS ON CACHE BOOL "" FORCE)
  set(ABSL_USE_EXTERNAL_GOOGLETEST ON CACHE BOOL "" FORCE)
else()
  set(ABSL_BUILD_TEST_HELPERS OFF CACHE BOOL "" FORCE)
  set(ABSL_USE_EXTERNAL_GOOGLETEST OFF CACHE BOOL "" FORCE)
endif()
set(ABSL_FIND_GOOGLETEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(absl)

# --- Protobuf ---
FetchContent_Declare(
  protobuf
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
  GIT_TAG        v33.4
)
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)
set(utf8_range_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(protobuf)

# --- RE2 ---
FetchContent_Declare(
  re2
  GIT_REPOSITORY https://github.com/google/re2.git
  GIT_TAG        2025-11-05
)
set(RE2_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(RE2_INSTALL OFF CACHE BOOL "" FORCE)
# RE2 doesn't have an ENABLE_INSTALL, we'll just ignore the errors if possible
# or try to satisfy it.
FetchContent_MakeAvailable(re2)



# --- ANTLR4 Runtime ---
FetchContent_Declare(
  antlr4_runtime
  GIT_REPOSITORY https://github.com/antlr/antlr4.git
  GIT_TAG        4.13.2
  SOURCE_SUBDIR  runtime/Cpp
)
set(ANTLR4_WITH_LIBCXX OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(antlr4_runtime)

# --- Flatbuffers ---
FetchContent_Declare(
  flatbuffers
  GIT_REPOSITORY https://github.com/google/flatbuffers.git
  GIT_TAG        v25.12.19
)
set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(flatbuffers)

# --- yaml-cpp ---
FetchContent_Declare(
  yaml_cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG        yaml-cpp-0.9.0
)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(yaml_cpp)

# --- cel-spec (for protos) ---
FetchContent_Declare(
  cel_spec
  GIT_REPOSITORY https://github.com/google/cel-spec.git
  GIT_TAG        v0.25.1
)
FetchContent_MakeAvailable(cel_spec)

# --- googleapis (for protos) ---
FetchContent_Declare(
  googleapis
  GIT_REPOSITORY https://github.com/googleapis/googleapis.git
  GIT_TAG        master # We might want a specific commit
)
FetchContent_MakeAvailable(googleapis)

# --- Helper for Protos ---
include(cmake/ProtobufGenerate.cmake)

# --- ANTLR4 Tool Management ---
set(ANTLR4_JAR_URL "https://www.antlr.org/download/antlr-4.13.2-complete.jar")
set(ANTLR4_JAR_PATH "${CMAKE_BINARY_DIR}/antlr-4.13.2-complete.jar")
if(NOT EXISTS "${ANTLR4_JAR_PATH}")
  message(STATUS "Downloading ANTLR4 jar...")
  file(DOWNLOAD "${ANTLR4_JAR_URL}" "${ANTLR4_JAR_PATH}" SHOW_PROGRESS)
endif()

find_package(Java REQUIRED)

# --- Include directories for the project ---
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${cel_spec_SOURCE_DIR})
include_directories(${googleapis_SOURCE_DIR})

# --- Protobuf Generation for external protos ---
set(PROTO_IMPORT_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${cel_spec_SOURCE_DIR}/proto
  ${googleapis_SOURCE_DIR}
  ${protobuf_SOURCE_DIR}/src
)

# cel-spec protos
file(GLOB_RECURSE CEL_SPEC_PROTOS "${cel_spec_SOURCE_DIR}/proto/cel/expr/*.proto")
protobuf_generate(
  LANGUAGE cpp
  PROTOS ${CEL_SPEC_PROTOS}
  OUT_VAR CEL_SPEC_PROTO_SRCS
  IMPORT_DIRS ${PROTO_IMPORT_DIRS}
  PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)
add_library(cel_spec_protos ${CEL_SPEC_PROTO_SRCS})
target_link_libraries(cel_spec_protos PUBLIC libprotobuf)
target_include_directories(cel_spec_protos PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# googleapis protos (just the ones we need for now)
file(GLOB_RECURSE GOOGLEAPIS_NEEDED_PROTOS
  "${googleapis_SOURCE_DIR}/google/rpc/*.proto"
  "${googleapis_SOURCE_DIR}/google/api/expr/v1alpha1/*.proto"
  "${googleapis_SOURCE_DIR}/google/type/*.proto"
)
protobuf_generate(
  LANGUAGE cpp
  PROTOS ${GOOGLEAPIS_NEEDED_PROTOS}
  OUT_VAR GOOGLEAPIS_PROTO_SRCS
  IMPORT_DIRS ${PROTO_IMPORT_DIRS}
  PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)
add_library(googleapis_protos ${GOOGLEAPIS_PROTO_SRCS})
target_link_libraries(googleapis_protos PUBLIC libprotobuf)
target_include_directories(googleapis_protos PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# --- Embed Tool ---
add_executable(cel_cc_embed bazel/cel_cc_embed.cc)
target_link_libraries(cel_cc_embed PRIVATE absl::flags absl::flags_parse absl::log absl::log_initialize absl::strings absl::span)

# --- Embedded Descriptor Sets ---
set(EMPTY_DESCRIPTOR_SET "${CMAKE_CURRENT_BINARY_DIR}/internal/empty_descriptor_set.pb")
set(EMPTY_DESCRIPTOR_SET_INC "${CMAKE_CURRENT_BINARY_DIR}/internal/empty_descriptor_set_embed.inc")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/internal")

add_custom_command(
  OUTPUT ${EMPTY_DESCRIPTOR_SET}
  COMMAND $<TARGET_FILE:protobuf::protoc> --descriptor_set_out=${EMPTY_DESCRIPTOR_SET} 
          -I${protobuf_SOURCE_DIR}/src
          ${protobuf_SOURCE_DIR}/src/google/protobuf/empty.proto
  DEPENDS protobuf::protoc
  COMMENT "Generating empty descriptor set"
)

add_custom_command(
  OUTPUT ${EMPTY_DESCRIPTOR_SET_INC}
  COMMAND $<TARGET_FILE:cel_cc_embed> --in=${EMPTY_DESCRIPTOR_SET} --out=${EMPTY_DESCRIPTOR_SET_INC}
  DEPENDS cel_cc_embed ${EMPTY_DESCRIPTOR_SET}
  COMMENT "Embedding empty descriptor set"
)

# Minimal descriptor set
set(MINIMAL_DESCRIPTOR_SET "${CMAKE_CURRENT_BINARY_DIR}/internal/minimal_descriptor_set.pb")
set(MINIMAL_DESCRIPTOR_SET_INC "${CMAKE_CURRENT_BINARY_DIR}/internal/minimal_descriptor_set_embed.inc")
add_custom_command(
  OUTPUT ${MINIMAL_DESCRIPTOR_SET}
  COMMAND $<TARGET_FILE:protobuf::protoc> --descriptor_set_out=${MINIMAL_DESCRIPTOR_SET} 
          -I${protobuf_SOURCE_DIR}/src
          ${protobuf_SOURCE_DIR}/src/google/protobuf/any.proto
          ${protobuf_SOURCE_DIR}/src/google/protobuf/duration.proto
          ${protobuf_SOURCE_DIR}/src/google/protobuf/struct.proto
          ${protobuf_SOURCE_DIR}/src/google/protobuf/timestamp.proto
          ${protobuf_SOURCE_DIR}/src/google/protobuf/wrappers.proto
  DEPENDS protobuf::protoc
  COMMENT "Generating minimal descriptor set"
)

add_custom_command(
  OUTPUT ${MINIMAL_DESCRIPTOR_SET_INC}
  COMMAND $<TARGET_FILE:cel_cc_embed> --in=${MINIMAL_DESCRIPTOR_SET} --out=${MINIMAL_DESCRIPTOR_SET_INC}
  DEPENDS cel_cc_embed ${MINIMAL_DESCRIPTOR_SET}
  COMMENT "Embedding minimal descriptor set"
)

# Testing descriptor set
set(TESTING_DESCRIPTOR_SET "${CMAKE_CURRENT_BINARY_DIR}/internal/testing_descriptor_set.pb")
set(TESTING_DESCRIPTOR_SET_INC "${CMAKE_CURRENT_BINARY_DIR}/internal/testing_descriptor_set_embed.inc")
add_custom_command(
  OUTPUT ${TESTING_DESCRIPTOR_SET}
  COMMAND $<TARGET_FILE:protobuf::protoc> --descriptor_set_out=${TESTING_DESCRIPTOR_SET} 
          --include_imports
          -I${protobuf_SOURCE_DIR}/src
          -I${cel_spec_SOURCE_DIR}/proto
          -I${CMAKE_CURRENT_SOURCE_DIR}
          ${cel_spec_SOURCE_DIR}/proto/cel/expr/conformance/proto2/test_all_types.proto
          ${cel_spec_SOURCE_DIR}/proto/cel/expr/conformance/proto2/test_all_types_extensions.proto
          ${cel_spec_SOURCE_DIR}/proto/cel/expr/conformance/proto3/test_all_types.proto
          ${cel_spec_SOURCE_DIR}/proto/cel/expr/checked.proto
          ${cel_spec_SOURCE_DIR}/proto/cel/expr/syntax.proto
          ${cel_spec_SOURCE_DIR}/proto/cel/expr/value.proto
          ${CMAKE_CURRENT_SOURCE_DIR}/eval/testutil/test_extensions.proto
          ${CMAKE_CURRENT_SOURCE_DIR}/eval/testutil/test_message.proto
          ${CMAKE_CURRENT_SOURCE_DIR}/testutil/test_json_names.proto
  DEPENDS protobuf::protoc
  COMMENT "Generating testing descriptor set"
)

add_custom_command(
  OUTPUT ${TESTING_DESCRIPTOR_SET_INC}
  COMMAND $<TARGET_FILE:cel_cc_embed> --in=${TESTING_DESCRIPTOR_SET} --out=${TESTING_DESCRIPTOR_SET_INC}
  DEPENDS cel_cc_embed ${TESTING_DESCRIPTOR_SET}
  COMMENT "Embedding testing descriptor set"
)

# --- Helper for Tests ---
if(BUILD_TESTING)
  function(cel_cc_test NAME)
    cmake_parse_arguments(ARG "" "" "SRCS;DEPS" ${ARGN})
    add_executable(${NAME} ${ARG_SRCS})
    target_link_libraries(${NAME} PRIVATE ${ARG_DEPS} GTest::gmock GTest::gtest_main)
    add_test(NAME cel_${NAME} COMMAND ${NAME} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  endfunction()

  add_library(cel_internal_testing STATIC 
    internal/testing.cc
    internal/testing_descriptor_pool.cc
    internal/testing_message_factory.cc
    internal/equals_text_proto.cc
    extensions/protobuf/memory_manager.cc
    runtime/internal/runtime_env_testing.cc
    common/value_testing.cc
    base/internal/memory_manager_testing.cc
    eval/public/testing/matchers.cc
    ${TESTING_DESCRIPTOR_SET_INC}
  )
  target_link_libraries(cel_internal_testing PUBLIC 
    GTest::gmock GTest::gtest absl::status_matchers absl::strings absl::status
    cel_internal cel_common
  )
  target_include_directories(cel_internal_testing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
endif()

# --- Components ---

# internal
set(CEL_INTERNAL_SRCS
  internal/new.cc
  internal/utf8.cc
  internal/strings.cc
  internal/lexis.cc
  internal/proto_time_encoding.cc
  internal/time.cc
  internal/names.cc
  internal/empty_descriptors.cc
  internal/minimal_descriptors.cc
  internal/well_known_types.cc
  internal/json.cc
  internal/message_equality.cc
  internal/overflow.cc
  internal/string_pool.cc
  extensions/protobuf/internal/map_reflection.cc
  extensions/protobuf/internal/qualify.cc
)
add_library(cel_internal ${CEL_INTERNAL_SRCS} ${EMPTY_DESCRIPTOR_SET_INC} ${MINIMAL_DESCRIPTOR_SET_INC})
target_include_directories(cel_internal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(cel_internal PUBLIC 
  absl::base absl::core_headers absl::status absl::strings absl::log absl::optional absl::no_destructor absl::any absl::variant absl::time 
  protobuf::libprotobuf googleapis_protos
)

# base
file(GLOB_RECURSE CEL_BASE_SRCS 
  "base/*.cc"
)
list(FILTER CEL_BASE_SRCS EXCLUDE REGEX ".*(test|benchmarks?|testing).*\\.cc$")
add_library(cel_base STATIC ${CEL_BASE_SRCS})
target_include_directories(cel_base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(cel_base PUBLIC cel_internal absl::base absl::core_headers absl::status absl::strings absl::time)

# common
file(GLOB_RECURSE CEL_COMMON_SRCS 
  "common/*.cc"
)
list(FILTER CEL_COMMON_SRCS EXCLUDE REGEX ".*(test|benchmarks?|testing).*\\.cc$")
add_library(cel_common STATIC ${CEL_COMMON_SRCS})
target_include_directories(cel_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(cel_common PUBLIC cel_base cel_internal cel_spec_protos googleapis_protos absl::base absl::core_headers absl::status absl::strings absl::log)

# parser
file(GLOB_RECURSE CEL_PARSER_SRCS 
  "parser/*.cc"
)
list(FILTER CEL_PARSER_SRCS EXCLUDE REGEX ".*(test|benchmarks?|testing).*\\.cc$")
set(ANTLR4_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/parser/internal")
file(MAKE_DIRECTORY ${ANTLR4_GEN_DIR})
set(ANTLR4_OUTPUTS
  ${ANTLR4_GEN_DIR}/CelLexer.h
  ${ANTLR4_GEN_DIR}/CelLexer.cpp
  ${ANTLR4_GEN_DIR}/CelParser.h
  ${ANTLR4_GEN_DIR}/CelParser.cpp
  ${ANTLR4_GEN_DIR}/CelVisitor.h
  ${ANTLR4_GEN_DIR}/CelVisitor.cpp
  ${ANTLR4_GEN_DIR}/CelBaseVisitor.h
  ${ANTLR4_GEN_DIR}/CelBaseVisitor.cpp
)

add_custom_command(
  OUTPUT ${ANTLR4_OUTPUTS}
  COMMAND ${Java_JAVA_EXECUTABLE} -jar "${ANTLR4_JAR_PATH}"
          -Werror -Dlanguage=Cpp -visitor -no-listener 
          -package "cel_parser_internal"
          -o "${ANTLR4_GEN_DIR}"
          "${CMAKE_CURRENT_SOURCE_DIR}/parser/internal/Cel.g4"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/parser/internal/Cel.g4" "${ANTLR4_JAR_PATH}"
  COMMENT "Generating ANTLR parsers for Cel"
)
add_library(cel_parser STATIC ${CEL_PARSER_SRCS} ${ANTLR4_OUTPUTS})
target_include_directories(cel_parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${ANTLR4_GEN_DIR})
target_link_libraries(cel_parser PUBLIC cel_common cel_internal antlr4_static absl::base absl::status absl::strings)

# checker
file(GLOB_RECURSE CEL_CHECKER_SRCS 
  "checker/*.cc"
)
list(FILTER CEL_CHECKER_SRCS EXCLUDE REGEX ".*(test|benchmarks?|testing).*\\.cc$")
add_library(cel_checker STATIC ${CEL_CHECKER_SRCS})
target_include_directories(cel_checker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(cel_checker PUBLIC cel_parser cel_common cel_internal absl::base absl::status absl::strings)

# runtime
file(GLOB_RECURSE CEL_RUNTIME_SRCS 
  "runtime/*.cc"
  "eval/*.cc"
)
list(FILTER CEL_RUNTIME_SRCS EXCLUDE REGEX ".*(test|benchmarks?|testing).*\\.cc$")
add_library(cel_runtime STATIC ${CEL_RUNTIME_SRCS})
target_include_directories(cel_runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(cel_runtime PUBLIC cel_checker cel_parser cel_common cel_internal re2::re2 absl::base absl::status absl::strings absl::log absl::flat_hash_map absl::flat_hash_set)

# extensions
file(GLOB_RECURSE CEL_EXTENSIONS_SRCS 
  "extensions/*.cc"
)
list(FILTER CEL_EXTENSIONS_SRCS EXCLUDE REGEX ".*(test|benchmarks?|testing).*\\.cc$")
add_library(cel_extensions STATIC ${CEL_EXTENSIONS_SRCS})
target_include_directories(cel_extensions PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(cel_extensions PUBLIC cel_runtime cel_internal absl::base absl::status absl::strings absl::log)


# compiler
file(GLOB_RECURSE CEL_COMPILER_SRCS "compiler/*.cc")
list(FILTER CEL_COMPILER_SRCS EXCLUDE REGEX ".*(test|benchmarks?|testing).*\\.cc$")
add_library(cel_compiler STATIC ${CEL_COMPILER_SRCS})
target_include_directories(cel_compiler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(cel_compiler PUBLIC cel_checker cel_parser cel_common cel_internal absl::base absl::status absl::strings absl::log)

# validator
file(GLOB_RECURSE CEL_VALIDATOR_SRCS "validator/*.cc")
list(FILTER CEL_VALIDATOR_SRCS EXCLUDE REGEX ".*(test|benchmarks?|testing).*\\.cc$")
add_library(cel_validator STATIC ${CEL_VALIDATOR_SRCS})
target_include_directories(cel_validator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(cel_validator PUBLIC cel_common cel_internal absl::base absl::status absl::strings)

# env
file(GLOB_RECURSE CEL_ENV_SRCS "env/*.cc")
list(FILTER CEL_ENV_SRCS EXCLUDE REGEX ".*(test|benchmarks?|testing).*\\.cc$")
add_library(cel_env STATIC ${CEL_ENV_SRCS})
target_include_directories(cel_env PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(cel_env PUBLIC cel_common cel_internal absl::base absl::status absl::strings yaml-cpp)

# tools
file(GLOB_RECURSE CEL_TOOLS_SRCS "tools/*.cc")
list(FILTER CEL_TOOLS_SRCS EXCLUDE REGEX ".*(test|benchmarks?|testing).*\\.cc$")
add_library(cel_tools STATIC ${CEL_TOOLS_SRCS})
target_include_directories(cel_tools PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(cel_tools PUBLIC cel_common cel_internal absl::base absl::status absl::strings re2::re2 flatbuffers)

# --- Test helper libraries ---

if(BUILD_TESTING)
  # testutil library (expr_printer used by parser tests)
  
  # testutil protos
  protobuf_generate(
    OUT_VAR TESTUTIL_PROTO_SRCS
    PROTOS 
      eval/tests/request_context.proto
      eval/testutil/test_message.proto
      eval/testutil/test_extensions.proto
      testutil/test_json_names.proto
    IMPORT_DIRS ${PROTO_IMPORT_DIRS}
  )
  add_library(cel_testutil STATIC testutil/expr_printer.cc testutil/baseline_tests.cc ${TESTUTIL_PROTO_SRCS})

  target_include_directories(cel_testutil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(cel_testutil PUBLIC cel_common cel_internal absl::strings absl::log)

  # checker internal testing helpers
  add_library(cel_checker_internal_testing STATIC checker/internal/test_ast_helpers.cc)
  target_include_directories(cel_checker_internal_testing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(cel_checker_internal_testing PUBLIC cel_checker cel_parser cel_common cel_internal absl::status)
endif()

# --- Tests ---
if(BUILD_TESTING)
include(${CMAKE_CURRENT_SOURCE_DIR}/cel_checker_tests.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cel_common_tests.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cel_env_tests.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cel_eval_tests.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cel_extensions_tests.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cel_runtime_tests.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cel_testutil_tests.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cel_tools_tests.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cel_validator_tests.cmake)
endif()
