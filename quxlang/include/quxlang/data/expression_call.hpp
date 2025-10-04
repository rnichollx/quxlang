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
