//
// Created by Ryan Nicholl on 7/24/23.
//

#ifndef RPNX_RYANSCRIPT1031_ENTITY_AST_HEADER
#define RPNX_RYANSCRIPT1031_ENTITY_AST_HEADER

#include "function_ast.hpp"
#include "rpnx/value.hpp"
#include "rylang/ast/class_entity_ast.hpp"
#include "rylang/ast/functum_entity_ast.hpp"
#include "rylang/ast/namespace_entity_ast.hpp"
#include "rylang/ast/variable_entity_ast.hpp"
#include "rylang/ast/null_object_ast.hpp"
#include "rylang/fwd.hpp"
#include <map>
#include <variant>

namespace rylang
{
    enum class entity_type { null_object_type, function_type, variable_type, namespace_type, class_type };
    struct entity_ast
    {
        // Fields
        std::map< std::string, rpnx::value< entity_ast > > m_sub_entities;
        bool m_is_field_entity = false;
        rpnx::value< std::variant< null_object_ast, functum_entity_ast, variable_entity_ast, namespace_entity_ast, class_entity_ast > > m_specialization;

        // member functions
        bool operator<(entity_ast const& other) const
        {
            return std::tie(m_sub_entities, m_specialization) < std::tie(other.m_sub_entities, other.m_specialization);
        }

        bool operator==(entity_ast const& other) const
        {
            return !(*this < other) && !(other < *this);
        }

        entity_ast& operator[](std::string const& str)
        {
            return m_sub_entities[str].get();
        }

        entity_ast const& operator[](std::string const& str) const
        {
            return m_sub_entities.at(str).get();
        }

        entity_ast() = default;
        entity_ast(functum_entity_ast const& other, bool field = false, std::map< std::string, rpnx::value< entity_ast > > const& sub_entities = {})
            : m_is_field_entity(field)
            , m_specialization(other)
            , m_sub_entities(sub_entities)
        {
        }

        entity_ast(variable_entity_ast const& other, bool field = false, std::map< std::string, rpnx::value< entity_ast > > const& sub_entities = {})
            : m_is_field_entity(field)
            , m_specialization(other)
            , m_sub_entities(sub_entities)
        {
        }

        entity_ast(namespace_entity_ast const& other, bool field = false, std::map< std::string, rpnx::value< entity_ast > > const& sub_entities = {})
            : m_is_field_entity(field)
            , m_specialization(other)
            , m_sub_entities(sub_entities)
        {
        }

        entity_ast(class_entity_ast const& other, bool field = false, std::map< std::string, rpnx::value< entity_ast > > const& sub_entities = {})
            : m_is_field_entity(field)
            , m_specialization(other)
            , m_sub_entities(sub_entities)
        {
        }

        entity_ast(null_object_ast const& other, bool field = false, std::map< std::string, rpnx::value< entity_ast > > const& sub_entities = {})
            : m_is_field_entity(field)
            , m_specialization(other)
            , m_sub_entities(sub_entities)
        {
        }

        std::string to_string() const;

        auto index() const
        {
            return m_specialization.get().index();
        }

        template < typename T >
        T const& get_as() const
        {
            return std::get< T >(m_specialization.get());
        }

        template < typename T >
        T& get_as()
        {
            return std::get< T >(m_specialization.get());
        }

        entity_type type() const
        {
            if (std::holds_alternative< null_object_ast >(m_specialization.get()))
            {
                return entity_type::null_object_type;
            }
            else if (std::holds_alternative< functum_entity_ast >(m_specialization.get()))
            {
                return entity_type::function_type;
            }
            else if (std::holds_alternative< variable_entity_ast >(m_specialization.get()))
            {
                return entity_type::variable_type;
            }
            else if (std::holds_alternative< namespace_entity_ast >(m_specialization.get()))
            {
                return entity_type::namespace_type;
            }
            else if (std::holds_alternative< class_entity_ast >(m_specialization.get()))
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
#include "rylang/ast/functum_entity_ast.hpp"
#include "rylang/ast/namespace_entity_ast.hpp"
#include "rylang/ast/null_object_ast.hpp"
#include "rylang/ast/variable_entity_ast.hpp"

#endif // RPNX_RYANSCRIPT1031_ENTITY_AST_HEADER
