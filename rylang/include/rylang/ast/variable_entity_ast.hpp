//
// Created by Ryan Nicholl on 7/29/23.
//

#ifndef RYLANG_VARIABLE_ENTITY_AST_HEADER_GUARD
#define RYLANG_VARIABLE_ENTITY_AST_HEADER_GUARD
#include <string>

namespace rylang
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
} // namespace rylang

#endif // RYLANG_VARIABLE_ENTITY_AST_HEADER_GUARD
