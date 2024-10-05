// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_BUILTINS_HEADER_GUARD
#define QUXLANG_DATA_BUILTINS_HEADER_GUARD

#include <string>

#include "type_symbol.hpp"
#include <rpnx/metadata.hpp>
namespace quxlang
{
    struct expression_target
    {
        std::string target;

        RPNX_MEMBER_METADATA(expression_target, target);
    };

    struct expression_sizeof
    {
        type_symbol what;

        RPNX_MEMBER_METADATA(expression_sizeof, what);
    };
} // namespace quxlang

#endif // RPNX_QUXLANG_BUILTINS_HEADER
