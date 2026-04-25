// Copyright 2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "eval/tools/proto_to_predicate.h"

#include <memory>
#include <string>

#include "absl/status/status.h"
#include "common/ast.h"
#include "common/ast_proto.h"
#include "compiler/compiler.h"
#include "env/config.h"
#include "env/env.h"
#include "env/env_std_extensions.h"
#include "env/env_yaml.h"
#include "eval/testutil/test_message.pb.h"
#include "internal/testing.h"
#include "internal/testing_descriptor_pool.h"
#include "tools/cel_unparser.h"

namespace cel::tools {
namespace {

using ::google::api::expr::runtime::TestMessage;

constexpr absl::string_view kEnvYaml = R"(
name: "test"
extensions:
  - name: "sets"
  - name: "two-var-comprehensions"
  - name: "bindings"
  - name: "optional"
variables:
  - name: "input"
    type: "eval.testutil.TestMessage"
)";

absl::StatusOr<std::unique_ptr<cel::Compiler>> CreateCompiler() {
  CEL_ASSIGN_OR_RETURN(cel::Config config, cel::EnvConfigFromYaml(std::string(kEnvYaml)));
  cel::Env env;
  cel::RegisterStandardExtensions(env);
  env.SetConfig(config);
  env.SetDescriptorPool(cel::internal::GetSharedTestingDescriptorPool());
  return env.NewCompiler();
}

TEST(ProtocolBufferToPredicateAstTest, PrimitivesTest) {
  TestMessage msg;
  msg.set_int32_value(42);
  msg.set_string_value("hello");

  ASSERT_OK_AND_ASSIGN(cel::Ast ast, ProtocolBufferToPredicateAst(msg, "input"));
  ASSERT_OK_AND_ASSIGN(auto compiler, CreateCompiler());

  auto result_or = compiler->GetTypeChecker().Check(std::make_unique<cel::Ast>(ast));
  ASSERT_OK(result_or);
  EXPECT_TRUE(result_or->IsValid());

  cel::expr::ParsedExpr parsed_expr;
  ASSERT_OK(cel::AstToParsedExpr(ast, &parsed_expr));
  ASSERT_OK_AND_ASSIGN(auto unparsed, google::api::expr::Unparse(parsed_expr));
  EXPECT_EQ(unparsed, "input.int32_value == 42 && input.string_value == \"hello\"");
}

TEST(ProtocolBufferToPredicateAstTest, RepeatedFieldTest) {
  TestMessage msg;
  msg.add_int32_list(1);
  msg.add_int32_list(2);

  ASSERT_OK_AND_ASSIGN(cel::Ast ast, ProtocolBufferToPredicateAst(msg, "input"));
  ASSERT_OK_AND_ASSIGN(auto compiler, CreateCompiler());

  auto result_or = compiler->GetTypeChecker().Check(std::make_unique<cel::Ast>(ast));
  ASSERT_OK(result_or);
  EXPECT_TRUE(result_or->IsValid());

  cel::expr::ParsedExpr parsed_expr;
  ASSERT_OK(cel::AstToParsedExpr(ast, &parsed_expr));
  ASSERT_OK_AND_ASSIGN(auto unparsed, google::api::expr::Unparse(parsed_expr));
  EXPECT_EQ(unparsed, "sets.contains(input.int32_list, [1, 2])");
}

TEST(ProtocolBufferToPredicateAstTest, MapFieldTest) {
  TestMessage msg;
  auto& map = *msg.mutable_string_int32_map();
  map["foo"] = 1;
  map["bar"] = 2;

  ASSERT_OK_AND_ASSIGN(cel::Ast ast, ProtocolBufferToPredicateAst(msg, "input"));
  ASSERT_OK_AND_ASSIGN(auto compiler, CreateCompiler());

  auto result_or = compiler->GetTypeChecker().Check(std::make_unique<cel::Ast>(ast));
  ASSERT_OK(result_or);
  EXPECT_TRUE(result_or->IsValid());

  cel::expr::ParsedExpr parsed_expr;
  ASSERT_OK(cel::AstToParsedExpr(ast, &parsed_expr));
  ASSERT_OK_AND_ASSIGN(auto unparsed, google::api::expr::Unparse(parsed_expr));
  EXPECT_THAT(unparsed, testing::AnyOf(
    testing::Eq("cel.bind(map_val, {\"bar\": 2, \"foo\": 1}, input.string_int32_map.all(k, v, map_val[?k].optMap(val, val == v).orValue(false)))"),
    testing::Eq("cel.bind(map_val, {\"foo\": 1, \"bar\": 2}, input.string_int32_map.all(k, v, map_val[?k].optMap(val, val == v).orValue(false)))")
  ));
}

TEST(ProtocolBufferToPredicateAstTest, MultipleMessagesTest) {
  TestMessage msg1;
  msg1.set_int32_value(42);

  TestMessage msg2;
  msg2.set_int32_value(41);
  msg2.set_string_value("hello");

  std::vector<const google::protobuf::Message*> messages = {&msg1, &msg2};

  ASSERT_OK_AND_ASSIGN(auto ast, ProtocolBufferToPredicateAst(absl::MakeSpan(messages)));

  cel::expr::ParsedExpr parsed_expr;
  ASSERT_OK(cel::AstToParsedExpr(ast, &parsed_expr));
  ASSERT_OK_AND_ASSIGN(auto unparsed, google::api::expr::Unparse(parsed_expr));
  EXPECT_EQ(unparsed, "input.int32_value == 42 || input.int32_value == 41 && input.string_value == \"hello\"");
}

}  // namespace
}  // namespace cel::tools
