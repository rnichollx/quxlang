//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef QUXLANG_FUNCTION_STATEMENT_HEADER_GUARD
#define QUXLANG_FUNCTION_STATEMENT_HEADER_GUARD

#include "quxlang/data/expression.hpp"
#include "quxlang/data/function_return_statement.hpp"
#include "rpnx/variant.hpp"
#include <boost/variant.hpp>
#include <tuple>
#include <utility>
#include <variant>

namespace quxlang
{
    struct function_expression_statement;
    struct function_block;
    struct function_if_statement;

    // TODO: Implement while
    struct function_while_statement;

    struct function_var_statement
    {
        std::string name;
        type_symbol type;
        // TODO: support named initializers
        std::vector< expression > initializers;

        RPNX_MEMBER_METADATA(function_var_statement, name, type, initializers);
    };

    using function_statement = rpnx::variant< std::monostate, function_block, function_expression_statement, function_if_statement, function_while_statement, function_var_statement, function_return_statement >;

    struct function_block
    {
        std::vector< function_statement > statements;

        RPNX_MEMBER_METADATA(function_block, statements);
    };

} // namespace quxlang

#include "quxlang/data/function_expression_statement.hpp"
#include "quxlang/data/function_if_statement.hpp"
#include "quxlang/data/function_while_statement.hpp"

#endif // QUXLANG_FUNCTION_STATEMENT_HEADER_GUARD
