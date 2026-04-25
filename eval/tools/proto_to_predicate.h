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

#ifndef THIRD_PARTY_CEL_CPP_EVAL_TOOLS_PROTO_TO_PREDICATE_H_
#define THIRD_PARTY_CEL_CPP_EVAL_TOOLS_PROTO_TO_PREDICATE_H_

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "common/ast.h"
#include "google/protobuf/message.h"

namespace cel::tools {

// Converts a protocol buffer into a CEL AST predicate.
//
// For every leaf value set in the protocol buffer message, this function
// converts the field and value into a predicate, logically ANDed (`&&`) together:
//   - Primitives: `input.field == <value>`
//   - Repeated fields: `sets.contains(<input>.field, [<value>])`
//   - Maps: `<input>.field.exists(k, v, k in <map> && <map[k]> == v)`
//
// The output is an AST representing this combined predicate. Type mapping 
// ensures that evaluating this AST on the original protobuf type will 
// type-check properly when using the `sets` CEL extension.
absl::StatusOr<cel::Ast> ProtocolBufferToPredicateAst(
    const google::protobuf::Message& message,
    absl::string_view input_name = "input");

absl::StatusOr<cel::Ast> ProtocolBufferToPredicateAst(
    absl::Span<const google::protobuf::Message* const> messages,
    absl::string_view input_name = "input");

}  // namespace cel::tools

#endif  // THIRD_PARTY_CEL_CPP_EVAL_TOOLS_PROTO_TO_PREDICATE_H_
