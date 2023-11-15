//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_MEMBER_VARIABLE_AST_HEADER
#define RPNX_RYANSCRIPT1031_MEMBER_VARIABLE_AST_HEADER

#include "rylang/data/qualified_symbol_reference.hpp"
#include <string>

namespace rylang
{
    struct member_variable_ast
    {
        std::string name;
        qualified_symbol_reference type;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_MEMBER_VARIABLE_AST_HEADER
