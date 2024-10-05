// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_EXPRESSION_EQUALS_HEADER_GUARD
#define QUXLANG_DATA_EXPRESSION_EQUALS_HEADER_GUARD

#include "expression.hpp"

namespace quxlang
{
    struct expression_equals
    {
        static constexpr const char * name = "equals";
        static constexpr const char * symbol = "==";
        static constexpr const int priority = 2;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(const expression_equals& other) const  = default;
    };

    struct expression_not_equals
    {
        static constexpr const char * name = "not_equals";
        static constexpr const char * symbol = "!=";
        static constexpr const int priority = 2;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(const expression_not_equals& other) const  = default;
    };
} // namespace quxlang

#endif // QUXLANG_EXPRESSION_EQUALS_HEADER_GUARD
