//
// Created by Ryan Nicholl on 10/26/23.
//

#ifndef RYLANG_EXPRESSION_BOOL_HEADER_GUARD
#define RYLANG_EXPRESSION_BOOL_HEADER_GUARD

#include "expression.hpp"
namespace rylang
{
    // operator && (logical and)
    struct expression_and
    {
        static constexpr const char* name = "logical_and";
        static constexpr const char* symbol = "&&";
        static constexpr const int priority = 1;
        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(const expression_and& other) const  = default;
    };

    // operator || (logical or)
    struct expression_or
    {
        static constexpr const char* name = "logical_or";
        static constexpr const char* symbol = "||";
        static constexpr const int priority = 1;
        expression lhs;
        expression rhs;
        std::strong_ordering operator<=>(const expression_or& other) const  = default;
    };


    // operator !| (logical nor)
    struct expression_nor
    {
        static constexpr const char* name = "logical_nor";
        static constexpr const char* symbol = "!|";
        static constexpr const int priority = 1;
        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(const expression_nor& other) const  = default;
    };

    // operator ^^ (logical xor)
    struct expression_xor
    {
        static constexpr const char* name = "logical_xor";
        static constexpr const char* symbol = "^^";
        static constexpr const int priority = 1;
        expression lhs;
        expression rhs;
        std::strong_ordering operator<=>(const expression_xor& other) const  = default;
    };

    // Operator !& (nand)
    struct expression_nand
    {
        static constexpr const char* name = "logical_nand";
        static constexpr const char* symbol = "!&";
        static constexpr const int priority = 1;
        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(const expression_nand& other) const  = default;
    };

    // operator ^> (logical implication)
    struct expression_implies
    {
        static constexpr const char* name = "logical_implies";
        static constexpr const char* symbol = "^>";
        static constexpr const int priority = 1;
        expression lhs;
        expression rhs;
        std::strong_ordering operator<=>(const expression_implies& other) const = default;
    };

    // operator ^< (logical reverse implication)
    struct expression_implied
    {
        static constexpr const char* name = "logical_implied";
        static constexpr const char* symbol = "^<";
        static constexpr const int priority = 1;
        expression lhs;
        expression rhs;
        std::strong_ordering operator<=>(const expression_implied& other) const = default;
    };
} // namespace rylang

#endif // RYLANG_EXPRESSION_BOOL_HEADER_GUARD
