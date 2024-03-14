//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef QUXLANG_MEMBER_VARIABLE_AST_HEADER_GUARD
#define QUXLANG_MEMBER_VARIABLE_AST_HEADER_GUARD

#include "quxlang/data/type_symbol.hpp"
#include <string>

namespace quxlang
{
    struct member_variable_ast
    {
        std::string name;
        type_symbol type;
    };
} // namespace quxlang

#endif // QUXLANG_MEMBER_VARIABLE_AST_HEADER_GUARD
