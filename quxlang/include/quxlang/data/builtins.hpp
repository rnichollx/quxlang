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
    struct target_expr
    {
        std::string target;

        RPNX_MEMBER_METADATA(target_expr, target);
    };

    struct sizeof_expr
    {
        type_symbol what;

        RPNX_MEMBER_METADATA(sizeof_expr, what);
    };
} // namespace quxlang

#endif // RPNX_QUXLANG_BUILTINS_HEADER
