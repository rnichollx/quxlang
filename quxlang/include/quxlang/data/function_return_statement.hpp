//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef QUXLANG_FUNCTION_RETURN_STATEMENT_HEADER_GUARD
#define QUXLANG_FUNCTION_RETURN_STATEMENT_HEADER_GUARD

#include "quxlang/data/expression.hpp"
#include <optional>

namespace quxlang
{
    struct function_return_statement
    {
        std::optional<expression> expr;

        RPNX_MEMBER_METADATA(function_return_statement, expr);
    };
} // namespace quxlang

#endif // QUXLANG_FUNCTION_RETURN_STATEMENT_HEADER_GUARD
