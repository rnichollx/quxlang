//
// Created by Ryan Nicholl on 10/26/23.
//

#ifndef RYLANG_EXPRESSION_COPY_ASSIGN_HEADER_GUARD
#define RYLANG_EXPRESSION_COPY_ASSIGN_HEADER_GUARD

#include "expression.hpp"
#include <compare>

namespace rylang
{
    struct expression_copy_assign
    {
        static constexpr const char* name = "copy_assign";
        static constexpr const char* symbol = ":=";
        static constexpr const int priority = 0;
        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(const expression_copy_assign& other) const  = default;
    };

} // namespace rylang

#endif // RYLANG_EXPRESSION_COPY_ASSIGN_HEADER_GUARD
