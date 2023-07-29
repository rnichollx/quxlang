//
// Created by Ryan Nicholl on 7/29/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_ENTITY_AST_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_ENTITY_AST_HEADER

#include "rylang/ast/entity_ast.hpp"
#include <string>

namespace rylang
{

    struct class_entity_ast
    {
        std::string to_string( entity_ast const * ) const;
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CLASS_ENTITY_AST_HEADER
