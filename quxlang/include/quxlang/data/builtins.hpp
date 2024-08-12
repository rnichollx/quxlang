//
// Created by Ryan Nicholl on 4/1/24.
//

#ifndef RPNX_QUXLANG_BUILTINS_HEADER
#define RPNX_QUXLANG_BUILTINS_HEADER

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
