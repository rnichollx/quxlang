//
// Created by Ryan Nicholl on 7/29/23.
//

#ifndef RPNX_RYANSCRIPT1031_ENTITY_BASE_AST_HEADER
#define RPNX_RYANSCRIPT1031_ENTITY_BASE_AST_HEADER
#include "function_ast.hpp"
#include "rpnx/value.hpp"
#include "rylang/ast/member_variable_ast.hpp"
#include "rylang/data/entity_category.hpp"
#include <map>

namespace rylang
{

    struct entity_ast
    {
        std::map< std::string, rpnx::value< entity_ast > > m_sub_entities;
        bool m_is_field_entity = false;
        std::string m_name;
        entity_category m_category = entity_category::unknown_cat;

        std::vector<function_ast> m_function_overloads;

        std::vector<member_variable_ast> m_member_variables;

        std::optional<type_ref_ast> m_variable_type;

        std::string to_string() const;
    };

}

#endif // RPNX_RYANSCRIPT1031_ENTITY_BASE_AST_HEADER
