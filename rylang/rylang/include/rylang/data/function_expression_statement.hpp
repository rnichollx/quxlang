//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RPNX_RYANSCRIPT1031_FUNCTION_EXPRESSION_STATEMENT_HEADER
#define RPNX_RYANSCRIPT1031_FUNCTION_EXPRESSION_STATEMENT_HEADER

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

#endif // RPNX_RYANSCRIPT1031_FUNCTION_EXPRESSION_STATEMENT_HEADER
