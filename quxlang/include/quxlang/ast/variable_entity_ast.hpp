//
// Created by Ryan Nicholl on 7/29/23.
//

#ifndef QUXLANG_VARIABLE_ENTITY_AST_HEADER_GUARD
#define QUXLANG_VARIABLE_ENTITY_AST_HEADER_GUARD
#include <string>

namespace quxlang
{
    struct variable_entity_ast
    {

        type_symbol m_variable_type;

        std::string to_string(entity_ast const* ) const
        {
            return "variable_ast{type=?}";
        }

        bool operator < (variable_entity_ast const& other) const
        {
            return m_variable_type < other.m_variable_type;
        }

    };
} // namespace quxlang

#endif // QUXLANG_VARIABLE_ENTITY_AST_HEADER_GUARD
