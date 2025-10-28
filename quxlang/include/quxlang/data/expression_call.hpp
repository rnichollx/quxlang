// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_EXPRESSION_CALL_HEADER_GUARD
#define QUXLANG_DATA_EXPRESSION_CALL_HEADER_GUARD

#include "quxlang/data/expression.hpp"
#include <vector>

namespace quxlang
{
    struct expression_arg
    {
        std::optional<std::string> name;
        expression value;

        RPNX_MEMBER_METADATA(expression_arg, name, value);
    };

    struct expression_call
    {
        expression callee;
        std::vector< expression_arg > args;

        RPNX_MEMBER_METADATA(expression_call, callee, args);
    };

    struct expression_bits
    {
        type_symbol of_type;

        RPNX_MEMBER_METADATA(expression_bits, of_type);
    };

    struct expression_sizeof
    {
        type_symbol of_type;

        RPNX_MEMBER_METADATA(expression_sizeof);
    };

    struct expression_is_integral
    {
        type_symbol of_type;

        RPNX_MEMBER_METADATA(expression_is_integral);
    };

    struct expression_is_signed
    {
        type_symbol of_type;

        RPNX_MEMBER_METADATA(expression_is_integral);
    };

    struct expression_is_same
    {
        type_symbol of_type;

        RPNX_MEMBER_METADATA(expression_is_same);
    };

    struct expression_typecast
    {
        expression expr;
        type_symbol to_type;
        std::optional< std::string > keyword;

        RPNX_MEMBER_METADATA(expression_typecast, expr, to_type, keyword);
    };


    struct delegate
    {
        // The name of the delegate
        std::string name;

        // TODO: Delegates should be able to refer to members by complex symbols

        // Expression arguments in a delegate call
        std::vector< expression_arg > args;

        RPNX_MEMBER_METADATA(delegate, name, args);
    };

} // namespace quxlang

#endif // QUXLANG_EXPRESSION_CALL_HEADER_GUARD
