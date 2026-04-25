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

#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "common/ast.h"
#include "common/expr.h"
#include "common/expr_factory.h"
#include "google/protobuf/message.h"
#include "google/protobuf/reflection.h"

namespace cel::tools {
namespace {

using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;

using FieldPath = std::vector<const FieldDescriptor*>;

class PredicateBuilder : public ExprFactory {
 public:
  explicit PredicateBuilder(absl::string_view input_name)
      : ExprFactory(), input_name_(input_name), id_(1) {}

  absl::StatusOr<Ast> Build(const Message& message) {
    std::vector<Expr> predicates;
    FieldPath path;

    auto status = Walk(message, path, predicates);
    if (!status.ok()) {
      return status;
    }

    if (predicates.empty()) {
      return Ast(NewBoolConst(NextId(), true), std::move(source_info_));
    }

    Expr root = FoldBinaryOp("_&&_", predicates);
    return Ast(std::move(root), std::move(source_info_));
  }

  absl::StatusOr<Ast> Build(
      absl::Span<const Message* const> messages) {
    if (messages.empty()) {
      return Ast(NewBoolConst(NextId(), true), std::move(source_info_));
    }

    std::vector<Expr> message_asts;
    for (const auto* message : messages) {
      auto predicate = Build(*message);
      if (!predicate.ok()) {
        return predicate.status();
      }
      message_asts.push_back(std::move(predicate->mutable_root_expr()));
    }

    Expr root = FoldBinaryOp("_||_", message_asts);
    return Ast(std::move(root), std::move(source_info_));
  }

 private:
  ExprId NextId() { return id_++; }

  // ---------------------------------------------------------------------------
  // Path construction
  // ---------------------------------------------------------------------------

  Expr BuildPath(ExprId expr_id, const FieldPath& path) {
    Expr e = NewIdent(expr_id, input_name_);
    for (const auto* f : path) {
      e = NewSelect(expr_id, std::move(e), f->name());
    }
    return e;
  }

  Expr BuildPath(const FieldPath& path) {
    return BuildPath(NextId(), path);
  }

  // ---------------------------------------------------------------------------
  // Field value extraction
  // ---------------------------------------------------------------------------

  // Converts a singular field value to a CEL constant expression.
  Expr PrimitiveToExpr(ExprId expr_id, const Message& message,
                       const Reflection* reflection,
                       const FieldDescriptor* field) {
    switch (field->cpp_type()) {
      case FieldDescriptor::CPPTYPE_INT32:
        return NewIntConst(expr_id, reflection->GetInt32(message, field));
      case FieldDescriptor::CPPTYPE_INT64:
        return NewIntConst(expr_id, reflection->GetInt64(message, field));
      case FieldDescriptor::CPPTYPE_UINT32:
        return NewUintConst(expr_id, reflection->GetUInt32(message, field));
      case FieldDescriptor::CPPTYPE_UINT64:
        return NewUintConst(expr_id, reflection->GetUInt64(message, field));
      case FieldDescriptor::CPPTYPE_DOUBLE:
        return NewDoubleConst(expr_id, reflection->GetDouble(message, field));
      case FieldDescriptor::CPPTYPE_FLOAT:
        return NewDoubleConst(expr_id, reflection->GetFloat(message, field));
      case FieldDescriptor::CPPTYPE_BOOL:
        return NewBoolConst(expr_id, reflection->GetBool(message, field));
      case FieldDescriptor::CPPTYPE_ENUM:
        return NewIntConst(expr_id, reflection->GetEnumValue(message, field));
      case FieldDescriptor::CPPTYPE_STRING: {
        std::string str_val = reflection->GetString(message, field);
        if (field->type() == FieldDescriptor::TYPE_BYTES) {
          return NewBytesConst(expr_id, std::move(str_val));
        }
        return NewStringConst(expr_id, std::move(str_val));
      }
      default:
        // Message is handled elsewhere
        break;
    }
    return NewNullConst(expr_id);
  }

  Expr PrimitiveToExpr(const Message& message, const Reflection* reflection,
                       const FieldDescriptor* field) {
    return PrimitiveToExpr(NextId(), message, reflection, field);
  }

  // Converts a repeated field element to a CEL constant expression.
  Expr RepeatedPrimitiveToExpr(const Message& message,
                               const Reflection* reflection,
                               const FieldDescriptor* field, int index) {
    const ExprId id = NextId();
    switch (field->cpp_type()) {
      case FieldDescriptor::CPPTYPE_INT32:
        return NewIntConst(id, reflection->GetRepeatedInt32(message, field, index));
      case FieldDescriptor::CPPTYPE_INT64:
        return NewIntConst(id, reflection->GetRepeatedInt64(message, field, index));
      case FieldDescriptor::CPPTYPE_UINT32:
        return NewUintConst(id, reflection->GetRepeatedUInt32(message, field, index));
      case FieldDescriptor::CPPTYPE_UINT64:
        return NewUintConst(id, reflection->GetRepeatedUInt64(message, field, index));
      case FieldDescriptor::CPPTYPE_DOUBLE:
        return NewDoubleConst(id, reflection->GetRepeatedDouble(message, field, index));
      case FieldDescriptor::CPPTYPE_FLOAT:
        return NewDoubleConst(id, reflection->GetRepeatedFloat(message, field, index));
      case FieldDescriptor::CPPTYPE_BOOL:
        return NewBoolConst(id, reflection->GetRepeatedBool(message, field, index));
      case FieldDescriptor::CPPTYPE_ENUM:
        return NewIntConst(id, reflection->GetRepeatedEnumValue(message, field, index));
      case FieldDescriptor::CPPTYPE_STRING: {
        std::string str_val = reflection->GetRepeatedString(message, field, index);
        if (field->type() == FieldDescriptor::TYPE_BYTES) {
          return NewBytesConst(id, std::move(str_val));
        }
        return NewStringConst(id, std::move(str_val));
      }
      default:
        break;
    }
    return NewNullConst(id);
  }

  // ---------------------------------------------------------------------------
  // Expression construction helpers
  // ---------------------------------------------------------------------------

  // Creates a binary operator call: `lhs <op> rhs`.
  Expr ConstructBinaryOp(absl::string_view op, Expr lhs, Expr rhs) {
    std::vector<Expr> args;
    args.push_back(std::move(lhs));
    args.push_back(std::move(rhs));
    return NewCall(NextId(), op, std::move(args));
  }

  Expr ConstructEquality(Expr lhs, Expr rhs) {
    return ConstructBinaryOp("_==_", std::move(lhs), std::move(rhs));
  }

  // Left-folds a vector of expressions with a binary operator.
  // Requires: `exprs` is non-empty.
  Expr FoldBinaryOp(absl::string_view op, std::vector<Expr>& exprs) {
    Expr root = std::move(exprs[0]);
    for (size_t i = 1; i < exprs.size(); ++i) {
      root = ConstructBinaryOp(op, std::move(root), std::move(exprs[i]));
    }
    return root;
  }

  // ---------------------------------------------------------------------------
  // Map literal construction
  // ---------------------------------------------------------------------------

  Expr ConstructMapLiteral(ExprId expr_id, const Reflection* reflection,
                           int size, const Message& message,
                           const FieldDescriptor* field) {
    const FieldDescriptor* const key_field =
        field->message_type()->FindFieldByName("key");
    const FieldDescriptor* const value_field =
        field->message_type()->FindFieldByName("value");

    std::vector<MapExprEntry> entries;
    for (int i = 0; i < size; ++i) {
      const Message& entry_msg =
          reflection->GetRepeatedMessage(message, field, i);
      const Reflection* const entry_ref = entry_msg.GetReflection();
      entries.push_back(NewMapEntry(
          NextId(), PrimitiveToExpr(entry_msg, entry_ref, key_field),
          PrimitiveToExpr(entry_msg, entry_ref, value_field)));
    }
    return NewMap(expr_id, std::move(entries));
  }

  // ---------------------------------------------------------------------------
  // Map field predicate (extracted from Walk for readability)
  // ---------------------------------------------------------------------------

  // Builds the predicate for a map field:
  //   cel.bind(
  //     map_val, <literal_map>,
  //     <input.field.path>.all(k, v,
  //       map_val[?k].optMap(val, val == v).orValue(false)))
  void WalkMapField(const Reflection* reflection, const Message& message,
                    const FieldDescriptor* field, const FieldPath& path,
                    int size, std::vector<Expr>& predicates) {
    auto next_id = [&]() { return NextId(); };

    // <literal_map>
    const ExprId literal_map_id = NextId();
    Expr literal_map =
        ConstructMapLiteral(literal_map_id, reflection, size, message, field);

    // Reusable sub-expression builders for macro tracking.
    // map_val[?k]
    auto opt_select_map_key = [&](ExprId expr_id) {
      return NewCall(expr_id, "_[?_]",
                     std::vector<Expr>{NewIdent(expr_id, "map_val"),
                                       NewIdent(expr_id, "k")});
    };
    // val == v
    auto map_val_equals_v = [&](ExprId expr_id) {
      std::vector<Expr> args;
      args.push_back(NewIdent(expr_id, "val"));
      args.push_back(NewIdent(expr_id, "v"));
      return NewCall(expr_id, "_==_", std::move(args));
    };

    // Stable IDs for sub-expressions referenced multiple times.
    const ExprId opt_map_select_key_id = NextId();
    const ExprId map_val_equals_v_id = NextId();

    // val = map_val[?k].value(), val == v
    Expr optmap_comp = NewBind(
        next_id, "val",
        NewMemberCall(NextId(), "value",
                      opt_select_map_key(opt_map_select_key_id),
                      std::vector<Expr>{}),
        map_val_equals_v(map_val_equals_v_id));

    // Expanded ternary for optMap:
    //   map_val[?k].hasValue()
    //     ? optional.of(<bind>)
    //     : optional.none()
    Expr expanded_optmap = NewCall(
        NextId(), "_?_:_",
        std::vector<Expr>{
            NewMemberCall(NextId(), "hasValue",
                          opt_select_map_key(opt_map_select_key_id),
                          std::vector<Expr>{}),
            NewCall(NextId(), "optional.of",
                    std::vector<Expr>{std::move(optmap_comp)}),
            NewCall(NextId(), "optional.none", std::vector<Expr>{})});

    // optMap macro tracker
    source_info_.mutable_macro_calls()[optmap_comp.id()] = NewMemberCall(
        NextId(), "optMap", opt_select_map_key(opt_map_select_key_id),
        std::vector<Expr>{NewIdent(NextId(), "val"),
                          map_val_equals_v(map_val_equals_v_id)});

    // .orValue(false)
    Expr condition_expanded = NewMemberCall(
        NextId(), "orValue", std::move(expanded_optmap),
        std::vector<Expr>{NewBoolConst(NextId(), false)});

    // .all comprehension expansion
    Expr step_all = NewCall(
        NextId(), "_&&_",
        std::vector<Expr>{NewIdent(NextId(), AccuVarName()),
                          std::move(condition_expanded)});
    Expr condition_not_strictly_false = NewCall(
        NextId(), "@not_strictly_false",
        std::vector<Expr>{NewIdent(NextId(), AccuVarName())});

    const ExprId path_id = NextId();
    Expr all_comp = NewComprehension(
        NextId(), "k", "v", BuildPath(path_id, path), AccuVarName(),
        NewBoolConst(NextId(), true), std::move(condition_not_strictly_false),
        std::move(step_all), NewIdent(NextId(), AccuVarName()));

    // all macro tracker
    Expr all_condition_macro = NewMemberCall(
        NextId(), "orValue", NewUnspecified(optmap_comp.id()),
        std::vector<Expr>{NewBoolConst(NextId(), false)});
    source_info_.mutable_macro_calls()[all_comp.id()] = NewMemberCall(
        NextId(), "all", BuildPath(path_id, path),
        std::vector<Expr>{NewIdent(NextId(), "k"), NewIdent(NextId(), "v"),
                          std::move(all_condition_macro)});

    // cel.bind comprehension expansion
    Expr bind_comp =
        NewBind(next_id, "map_val", literal_map, std::move(all_comp));

    // bind macro tracker
    source_info_.mutable_macro_calls()[bind_comp.id()] = NewMemberCall(
        NextId(), "bind", NewIdent(NextId(), "cel"),
        std::vector<Expr>{NewIdent(NextId(), "map_val"), literal_map,
                          NewUnspecified(all_comp.id())});

    predicates.push_back(std::move(bind_comp));
  }

  // ---------------------------------------------------------------------------
  // Repeated field predicate (extracted from Walk for readability)
  // ---------------------------------------------------------------------------

  // Builds: sets.contains(<input>.field, [<values>])
  absl::Status WalkRepeatedField(const Reflection* reflection,
                                 const Message& message,
                                 const FieldDescriptor* field,
                                 const FieldPath& path, int size,
                                 std::vector<Expr>& predicates) {
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      return absl::UnimplementedError(
          "Repeated messages are not fully supported yet.");
    }

    std::vector<ListExprElement> elements;
    for (int i = 0; i < size; ++i) {
      elements.push_back(
          NewListElement(RepeatedPrimitiveToExpr(message, reflection, field, i)));
    }
    Expr literal_list = NewList(NextId(), std::move(elements));

    std::vector<Expr> contains_args;
    contains_args.push_back(BuildPath(path));
    contains_args.push_back(std::move(literal_list));
    predicates.push_back(
        NewCall(NextId(), "sets.contains", std::move(contains_args)));

    return absl::OkStatus();
  }

  // ---------------------------------------------------------------------------
  // Recursive message walk
  // ---------------------------------------------------------------------------

  absl::Status Walk(const Message& message, FieldPath& path,
                    std::vector<Expr>& predicates) {
    const Reflection* const reflection = message.GetReflection();
    std::vector<const FieldDescriptor*> fields;
    reflection->ListFields(message, &fields);

    for (const auto* field : fields) {
      path.push_back(field);

      if (field->is_map()) {
        const int size = reflection->FieldSize(message, field);
        if (size > 0) {
          WalkMapField(reflection, message, field, path, size, predicates);
        }
      } else if (field->is_repeated()) {
        const int size = reflection->FieldSize(message, field);
        if (size > 0) {
          auto status =
              WalkRepeatedField(reflection, message, field, path, size, predicates);
          if (!status.ok()) return status;
        }
      } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
        const Message& sub_message = reflection->GetMessage(message, field);
        auto status = Walk(sub_message, path, predicates);
        if (!status.ok()) return status;
      } else {
        // Primitive field: input.field == <value>
        predicates.push_back(ConstructEquality(
            BuildPath(path), PrimitiveToExpr(message, reflection, field)));
      }

      path.pop_back();
    }
    return absl::OkStatus();
  }

  absl::string_view input_name_;
  ExprId id_;
  SourceInfo source_info_;
};

}  // namespace

absl::StatusOr<Ast> ProtocolBufferToPredicateAst(const Message& message,
                                                 absl::string_view input_name) {
  PredicateBuilder builder(input_name);
  return builder.Build(message);
}

absl::StatusOr<Ast> ProtocolBufferToPredicateAst(
    absl::Span<const Message* const> messages,
    absl::string_view input_name) {
  PredicateBuilder builder(input_name);
  return builder.Build(messages);
}

}  // namespace cel::tools
