// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_EXPRESSION_MOVE_ASSIGN_HEADER_GUARD
#define QUXLANG_DATA_EXPRESSION_MOVE_ASSIGN_HEADER_GUARD

#include "expression.hpp"
namespace quxlang
{
    struct expression_move_assign
    {
        static constexpr const char* name = "move_assign";
        static constexpr const char* symbol = ":<";
        static constexpr const int priority = 0;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(const expression_move_assign& other) const  = default;
    };
} // namespace quxlang

#endif // QUXLANG_EXPRESSION_MOVE_ASSIGN_HEADER_GUARD
