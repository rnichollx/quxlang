//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_FUNCTION_RETURN_STATEMENT_HEADER
#define RPNX_RYANSCRIPT1031_FUNCTION_RETURN_STATEMENT_HEADER

#include "rylang/data/expression.hpp"
#include <optional>

namespace rylang
{
    struct function_return_statement
    {
        std::optional<expression> expr;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FUNCTION_RETURN_STATEMENT_HEADER
