//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_MEMBER_VARIABLE_AST_HEADER
#define RPNX_RYANSCRIPT1031_MEMBER_VARIABLE_AST_HEADER

#include <string>
#include "rylang/data/qualified_reference.hpp"

namespace rylang
{
    struct member_variable_ast
    {
        std::string name;
        qualified_symbol_reference type;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_MEMBER_VARIABLE_AST_HEADER
