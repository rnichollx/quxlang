//
// Created by Ryan Nicholl on 7/29/23.
//

#ifndef RPNX_RYANSCRIPT1031_VARIABLE_ENTITY_AST_HEADER
#define RPNX_RYANSCRIPT1031_VARIABLE_ENTITY_AST_HEADER
#include <string>

namespace rylang
{
    struct variable_entity_ast
    {

        type_reference m_variable_type;

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

#endif // RPNX_RYANSCRIPT1031_VARIABLE_ENTITY_AST_HEADER
