//
// Created by Ryan Nicholl on 10/26/23.
//

#ifndef RPNX_RYANSCRIPT1031_EXPRESSION_BOOL_HEADER
#define RPNX_RYANSCRIPT1031_EXPRESSION_BOOL_HEADER

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
    };

    // operator || (logical or)
    struct expression_or
    {
        static constexpr const char* name = "logical_or";
        static constexpr const char* symbol = "||";
        static constexpr const int priority = 1;
        expression lhs;
        expression rhs;
    };


    // operator !| (logical nor)
    struct expression_nor
    {
        static constexpr const char* name = "logical_nor";
        static constexpr const char* symbol = "!|";
        static constexpr const int priority = 1;
        expression lhs;
        expression rhs;
    };

    // operator ^^ (logical xor)
    struct expression_xor
    {
        static constexpr const char* name = "logical_xor";
        static constexpr const char* symbol = "^^";
        static constexpr const int priority = 1;
        expression lhs;
        expression rhs;
    };

    // Operator !& (nand)
    struct expression_nand
    {
        static constexpr const char* name = "logical_nand";
        static constexpr const char* symbol = "!&";
        static constexpr const int priority = 1;
        expression lhs;
        expression rhs;
    };

    // operator ^> (logical implication)
    struct expression_implies
    {
        static constexpr const char* name = "logical_implies";
        static constexpr const char* symbol = "^>";
        static constexpr const int priority = 1;
        expression lhs;
        expression rhs;
    };

    // operator ^< (logical reverse implication)
    struct expression_implied
    {
        static constexpr const char* name = "logical_implied";
        static constexpr const char* symbol = "^<";
        static constexpr const int priority = 1;
        expression lhs;
        expression rhs;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_EXPRESSION_BOOL_HEADER
