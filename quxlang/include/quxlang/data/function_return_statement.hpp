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

        std::strong_ordering operator<=>(const function_return_statement& other) const = default;
        inline bool operator==(const function_return_statement& other) const = default;
        inline bool operator!=(const function_return_statement& other) const = default;
    };
} // namespace quxlang

#endif // QUXLANG_FUNCTION_RETURN_STATEMENT_HEADER_GUARD
