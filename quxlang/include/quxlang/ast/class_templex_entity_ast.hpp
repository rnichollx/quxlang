//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef CLASS_TEMPLEX_ENTITY_AST_HEADER_GUARD
#define CLASS_TEMPLEX_ENTITY_AST_HEADER_GUARD
#include <vector>

#include "class_template_ast.hpp"

namespace quxlang
{
    struct class_templex_entity_ast
    {
        std::vector< class_template_ast > m_class_template_overloads;
    };
} // namespace quxlang

#endif // CLASS_TEMPLEX_ENTITY_AST_HEADER_GUARD
