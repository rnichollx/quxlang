//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_MEMBER_VARAIBLE_AST_HEADER
#define RPNX_RYANSCRIPT1031_MEMBER_VARAIBLE_AST_HEADER

#include "type_ref_ast.hpp"
#include <string>

namespace rylang
{
    struct member_variable_ast
    {
        std::string name;
        type_ref_ast type;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_MEMBER_VARAIBLE_AST_HEADER
