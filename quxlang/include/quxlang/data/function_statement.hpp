// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_FUNCTION_STATEMENT_HEADER_GUARD
#define QUXLANG_DATA_FUNCTION_STATEMENT_HEADER_GUARD

#include "quxlang/ast2/source_location.hpp"
#include <quxlang/data/basic_types.hpp>
#include "quxlang/data/function_return_statement.hpp"
#include "quxlang/data/statements.hpp"
#include "rpnx/variant.hpp"
#include <rpnx/macros.hpp>

#include <quxlang/ast2/source_location.hpp>

#include <tuple>
#include <utility>
#include <variant>
#include <cstdint>

RPNX_ENUM(quxlang, runtime_condition, std::uint16_t, CONSTEXPR, NATIVE);

/// Function-local compile-time storage class for STATIC and STATIC_VAR declarations.
RPNX_ENUM(quxlang, function_static_kind, std::uint16_t, constant, mutable_);

namespace quxlang
{
    struct function_expression_statement;
    struct function_block;
    struct function_if_statement;

    struct function_while_statement;
    struct function_for_statement;
    struct function_assert_statement;
    struct function_var_statement;
    struct function_unimplemented_statement;
    struct function_place_statement;
    struct function_destroy_statement;
    struct function_runtime_statement;
    struct function_return_statement;
    struct function_static_eval_statement;
    struct function_static_if_statement;
    struct function_static_while_statement;
    struct function_break_statement;
    struct function_continue_statement;


    using function_statement = rpnx::variant< function_block, function_expression_statement, function_if_statement, function_while_statement, function_for_statement, function_var_statement, function_return_statement, function_assert_statement, function_unimplemented_statement, function_place_statement, function_destroy_statement, function_runtime_statement, function_static_eval_statement, function_static_if_statement, function_static_while_statement, function_break_statement, function_continue_statement >;


    struct function_var_statement
    {
        std::string name;
        type_symbol type;
        std::vector< expression_arg > initializers;

        std::optional< expression > equals_initializer;
        /// Storage class for function-local STATIC/STATIC_VAR declarations; null for VAR.
        std::optional< function_static_kind > static_kind;

        QUX_AST_METADATA(function_var_statement, name, type, initializers, equals_initializer, static_kind);
    };

    struct function_unimplemented_statement
    {
        std::optional< std::string > error_message;

        QUX_AST_METADATA(function_unimplemented_statement, error_message);
    };

    struct function_block
    {
        std::vector< function_statement > statements;
        std::string block_dbg_string;

        QUX_AST_METADATA(function_block, statements, block_dbg_string);
    };

    struct function_assert_statement
    {
        expression condition;
        std::optional< std::string > tagline;

        QUX_AST_METADATA(function_assert_statement, condition, tagline);
    };

    // Runtime statement and condition support

    struct function_runtime_statement
    {
        runtime_condition condition;
        function_block then_block;
        std::optional< function_block > else_block;

        QUX_AST_METADATA(function_runtime_statement, condition, then_block, else_block);
    };

    struct function_expression_statement
    {
        expression expr;

        QUX_AST_METADATA(function_expression_statement, expr);
    };

    struct function_static_eval_statement
    {
        /// Expression evaluated immediately during VMIR generation.
        expression expr;

        QUX_AST_METADATA(function_static_eval_statement, expr);
    };

    struct function_if_statement
    {
        expression condition;
        function_block then_block;
        std::optional< function_block > else_block;

        QUX_AST_METADATA(function_if_statement, condition, then_block, else_block);
    };

    struct function_static_if_statement
    {
        /// Compile-time condition evaluated before choosing which block to generate.
        expression condition;
        /// Block generated when condition evaluates to true.
        function_block then_block;
        /// Optional STATIC_ELSE block generated when condition evaluates to false.
        std::optional< function_block > else_block;

        QUX_AST_METADATA(function_static_if_statement, condition, then_block, else_block);
    };


    struct function_while_statement
    {
        expression condition;
        function_block loop_block;

        QUX_AST_METADATA(function_while_statement, condition, loop_block);
    };

    struct function_for_statement
    {
        std::optional< function_block > init_block;
        std::optional< function_block > eval_block;
        std::optional< expression > test_condition;
        std::optional< expression > posttest_condition;
        std::optional< function_block > step_block;

        std::optional< std::string > iter_name;
        std::optional< std::string > value_name;
        std::optional< std::string > index_name;
        std::optional< std::string > item_name;

        std::optional< expression > in_expr;
        std::optional< expression > start_expr;
        std::optional< expression > end_expr;
        std::optional< expression > limit_expr;
        std::optional< expression > filter_expr;
        std::optional< expression > by_expr;
        std::optional< expression > from_expr;
        std::optional< expression > to_expr;
        std::optional< expression > until_expr;

        function_block loop_block;

        QUX_AST_METADATA(function_for_statement, init_block, eval_block, test_condition, posttest_condition, step_block, iter_name, value_name, index_name, item_name, in_expr, start_expr, end_expr, limit_expr, filter_expr, by_expr, from_expr, to_expr, until_expr, loop_block);
    };

    struct function_static_while_statement
    {
        /// Compile-time condition re-evaluated before each generated iteration.
        expression condition;
        /// Block generated once per true compile-time condition result.
        function_block loop_block;

        QUX_AST_METADATA(function_static_while_statement, condition, loop_block);
    };

    struct function_break_statement
    {
        QUXLANG_WITH_SOURCE_LOCATION_EMPTY_METADATA(function_break_statement);
    };

    struct function_continue_statement
    {
        QUXLANG_WITH_SOURCE_LOCATION_EMPTY_METADATA(function_continue_statement);
    };

    struct function_return_statement
    {
        std::optional<expression> expr;

        QUX_AST_METADATA(function_return_statement, expr);
    };

    struct function_place_statement
    {
        // The expression which yields a pointer to the location to place the object.
        expression at;

        // Type to place.
        type_symbol type;

        // Optional assignment initializer,
        // If present, args must be empty.
        // e.g. PLACE AT(loc) type := assign_init_expr;
        std::optional<expression> assign_init;

        // Optional constructor args,
        // e.g. PLACE AT(loc) type :(args...);
        std::vector<expression_arg> args;

        QUX_AST_METADATA(function_place_statement, at, type, assign_init, args);
    };

    struct function_destroy_statement
    {
        // The expression which yields the storage location to destroy the object within.
        expression at;
        // Type to destroy
        type_symbol type;
        // Optional destructor args.
        std::vector< expression_arg > args;

        QUX_AST_METADATA(function_destroy_statement, at, type, args);
    };
} // namespace quxlang

#include "quxlang/data/function_while_statement.hpp"

namespace quxlang
{
    std::optional<source_location> get_location(function_statement const& st);
    inline std::optional<source_location> get_location(function_statement const& st)
    {
        return rpnx::apply_visitor<std::optional<source_location>>(st, [](auto const& s) { return s.location; });
    }
} // namespace quxlang

#endif // QUXLANG_FUNCTION_STATEMENT_HEADER_GUARD
