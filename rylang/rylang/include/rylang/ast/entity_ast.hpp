//
// Created by Ryan Nicholl on 7/24/23.
//

#ifndef RPNX_RYANSCRIPT1031_ENTITY_AST_HEADER
#define RPNX_RYANSCRIPT1031_ENTITY_AST_HEADER

#include "function_ast.hpp"
#include "rpnx/value.hpp"
#include "rylang/ast/class_entity_ast.hpp"
#include "rylang/ast/function_entity_ast.hpp"
#include "rylang/ast/namespace_entity_ast.hpp"
#include "rylang/ast/variable_entity_ast.hpp"
#include "rylang/fwd.hpp"
#include <map>
#include <variant>

namespace rylang
{
    enum class entity_type { null_object_type, function_type, variable_type, namespace_type, class_type };
    struct entity_ast
    {
        std::map< std::string, rpnx::value< entity_ast > > m_sub_entities;

        bool operator<(entity_ast const& other) const
        {
            return std::tie(m_sub_entities, m_subvalue) < std::tie(other.m_sub_entities, other.m_subvalue);
        }

        bool m_is_field_entity = false;
        // std::string m_name;

        rpnx::value< std::variant< null_object_ast, function_entity_ast, variable_entity_ast, namespace_entity_ast, class_entity_ast > > m_subvalue;
        std::string to_string() const;

        auto index() const
        {
            return m_subvalue.get().index();
        }

        entity_type type() const
        {
            if (std::holds_alternative< null_object_ast >(m_subvalue.get()))
            {
                return entity_type::null_object_type;
            }
            else if (std::holds_alternative< function_entity_ast >(m_subvalue.get()))
            {
                return entity_type::function_type;
            }
            else if (std::holds_alternative< variable_entity_ast >(m_subvalue.get()))
            {
                return entity_type::variable_type;
            }
            else if (std::holds_alternative< namespace_entity_ast >(m_subvalue.get()))
            {
                return entity_type::namespace_type;
            }
            else if (std::holds_alternative< class_entity_ast >(m_subvalue.get()))
            {
                return entity_type::class_type;
            }
            else
            {
                throw std::runtime_error("Invalid entity type");
            }
        }
    };

} // namespace rylang

#include "rylang/ast/class_entity_ast.hpp"
#include "rylang/ast/function_entity_ast.hpp"
#include "rylang/ast/namespace_entity_ast.hpp"
#include "rylang/ast/null_object_ast.hpp"
#include "rylang/ast/variable_entity_ast.hpp"

#endif // RPNX_RYANSCRIPT1031_ENTITY_AST_HEADER
