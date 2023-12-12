//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RYLANG_FUNCTION_RETURN_STATEMENT_HEADER_GUARD
#define RYLANG_FUNCTION_RETURN_STATEMENT_HEADER_GUARD

#include "rylang/data/expression.hpp"
#include <optional>

namespace rylang
{
    struct function_return_statement
    {
        std::optional<expression> expr;

        std::strong_ordering operator<=>(const function_return_statement& other) const = default;
        inline bool operator==(const function_return_statement& other) const = default;
        inline bool operator!=(const function_return_statement& other) const = default;
    };
} // namespace rylang

#endif // RYLANG_FUNCTION_RETURN_STATEMENT_HEADER_GUARD
