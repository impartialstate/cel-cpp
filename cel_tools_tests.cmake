# --- tools tests ---

cel_cc_test(
  tools_branch_coverage_test
  SRCS
    tools/branch_coverage_test.cc
  DEPS
    cel_common
    cel_runtime
    cel_checker
    cel_parser
    cel_extensions
    cel_compiler
    cel_validator
    cel_env
    cel_tools
    cel_testutil
    cel_checker_internal_testing
    cel_internal_testing
)
cel_cc_test(
  tools_cel_field_extractor_test
  SRCS
    tools/cel_field_extractor_test.cc
  DEPS
    cel_common
    cel_runtime
    cel_checker
    cel_parser
    cel_extensions
    cel_compiler
    cel_validator
    cel_env
    cel_tools
    cel_testutil
    cel_checker_internal_testing
    cel_internal_testing
)
cel_cc_test(
  tools_cel_unparser_test
  SRCS
    tools/cel_unparser_test.cc
  DEPS
    cel_common
    cel_runtime
    cel_checker
    cel_parser
    cel_extensions
    cel_compiler
    cel_validator
    cel_env
    cel_tools
    cel_testutil
    cel_checker_internal_testing
    cel_internal_testing
)
cel_cc_test(
  tools_descriptor_pool_builder_test
  SRCS
    tools/descriptor_pool_builder_test.cc
  DEPS
    cel_common
    cel_runtime
    cel_checker
    cel_parser
    cel_extensions
    cel_compiler
    cel_validator
    cel_env
    cel_tools
    cel_testutil
    cel_checker_internal_testing
    cel_internal_testing
)
# cel_cc_test(
#   tools_flatbuffers_backed_impl_test
#   SRCS
#     tools/flatbuffers_backed_impl_test.cc
#   DEPS
#     cel_common
#     cel_runtime
#     cel_checker
#     cel_parser
#     cel_extensions
#     cel_compiler
#     cel_validator
#     cel_env
#     cel_tools
#     cel_testutil
#     cel_checker_internal_testing
#     cel_internal_testing
# )
cel_cc_test(
  tools_navigable_ast_test
  SRCS
    tools/navigable_ast_test.cc
  DEPS
    cel_common
    cel_runtime
    cel_checker
    cel_parser
    cel_extensions
    cel_compiler
    cel_validator
    cel_env
    cel_tools
    cel_testutil
    cel_checker_internal_testing
    cel_internal_testing
)
