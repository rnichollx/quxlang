//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RYLANG_MEMBER_VARIABLE_AST_HEADER_GUARD
#define RYLANG_MEMBER_VARIABLE_AST_HEADER_GUARD

#include "rylang/data/qualified_symbol_reference.hpp"
#include <string>

namespace rylang
{
    struct member_variable_ast
    {
        std::string name;
        type_symbol type;
    };
} // namespace rylang

#endif // RYLANG_MEMBER_VARIABLE_AST_HEADER_GUARD
