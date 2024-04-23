//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef QUXLANG_FUNCTION_EXPRESSION_STATEMENT_HEADER_GUARD
#define QUXLANG_FUNCTION_EXPRESSION_STATEMENT_HEADER_GUARD

#include "expression.hpp"
#include "quxlang/ordering.hpp"
#include <compare>

namespace quxlang
{
    struct function_expression_statement
    {
        expression expr;

        RPNX_MEMBER_METADATA(function_expression_statement, expr);
    };
} // namespace quxlang

#endif // QUXLANG_FUNCTION_EXPRESSION_STATEMENT_HEADER_GUARD
