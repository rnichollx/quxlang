// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_FUNCTION_STATEMENT_HEADER_GUARD
#define QUXLANG_DATA_FUNCTION_STATEMENT_HEADER_GUARD

#include "quxlang/ast2/source_location.hpp"
#include "quxlang/data/expression.hpp"
#include "quxlang/data/function_return_statement.hpp"
#include "rpnx/variant.hpp"

#include <tuple>
#include <utility>
#include <variant>

namespace quxlang
{
    struct function_expression_statement;
    struct function_block;
    struct function_if_statement;

    struct function_while_statement;
    struct function_assert_statement;
    struct function_var_statement;


    using function_statement = rpnx::variant< function_block, function_expression_statement, function_if_statement, function_while_statement, function_var_statement, function_return_statement, function_assert_statement >;


    struct function_var_statement
    {
        std::string name;
        type_symbol type;
        // TODO: support named initializers
        std::vector< expression > initializers;

        // TODO: implement parsing for this:
        std::optional< expression > equals_initializer;

        RPNX_MEMBER_METADATA(function_var_statement, name, type, initializers, equals_initializer);
    };

    struct function_block
    {
        std::vector< function_statement > statements;
        std::string block_dbg_string;

        RPNX_MEMBER_METADATA(function_block, statements, block_dbg_string);
    };

    struct function_assert_statement
    {
        expression condition;
        std::optional< std::string > tagline;
        ast2_source_location location;

        RPNX_MEMBER_METADATA(function_assert_statement, condition, tagline, location);
    };


} // namespace quxlang

#include "quxlang/data/function_expression_statement.hpp"
#include "quxlang/data/function_if_statement.hpp"
#include "quxlang/data/function_while_statement.hpp"

#endif // QUXLANG_FUNCTION_STATEMENT_HEADER_GUARD
