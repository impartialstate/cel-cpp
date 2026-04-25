# Copyright 2024 Google LLC
# ...
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "proto_to_predicate",
    srcs = ["proto_to_predicate.cc"],
    hdrs = ["proto_to_predicate.h"],
    deps = [
        "//common:ast",
        "//common:expr",
        "//common:expr_factory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "proto_to_predicate_test",
    srcs = ["proto_to_predicate_test.cc"],
    deps = [
        ":proto_to_predicate",
        "//common:ast",
        "//common:ast_proto",
        "//env",
        "//env:env_yaml",
        "//env:env_std_extensions",
        "//tools:cel_unparser",
        "//compiler",
        "//checker:type_checker_builder",
        "//parser",
        "//eval/testutil:test_message_cc_proto",
        "//internal:testing",
        "//internal:testing_descriptor_pool",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)
