//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RYLANG_FUNCTION_EXPRESSION_STATEMENT_HEADER_GUARD
#define RYLANG_FUNCTION_EXPRESSION_STATEMENT_HEADER_GUARD

#include "expression.hpp"
#include "rylang/ordering.hpp"
#include <compare>

namespace rylang
{
    struct function_expression_statement
    {
        expression expr;

        std::strong_ordering operator<=>(const function_expression_statement& other) const = default;


    };
} // namespace rylang

#endif // RYLANG_FUNCTION_EXPRESSION_STATEMENT_HEADER_GUARD
