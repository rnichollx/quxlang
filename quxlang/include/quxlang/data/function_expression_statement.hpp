// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_FUNCTION_EXPRESSION_STATEMENT_HEADER_GUARD
#define QUXLANG_DATA_FUNCTION_EXPRESSION_STATEMENT_HEADER_GUARD

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
