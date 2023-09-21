//
// Created by Ryan Nicholl on 7/24/23.
//

#ifndef RPNX_RYANSCRIPT1031_ENTITY_AST_HEADER
#define RPNX_RYANSCRIPT1031_ENTITY_AST_HEADER

#include "function_ast.hpp"
#include "rpnx/value.hpp"
#include "rylang/fwd.hpp"
#include <map>
#include <variant>

namespace rylang
{
    struct entity_ast
    {
        std::map< std::string, rpnx::value< entity_ast > > m_sub_entities;

        bool m_is_field_entity = false;
        std::string m_name;
        rpnx::value< std::variant< null_object_ast, function_entity_ast, variable_entity_ast, namespace_entity_ast, class_entity_ast > > m_subvalue;
        std::string to_string() const;
    };

} // namespace rylang

#include "rylang/ast/class_entity_ast.hpp"
#include "rylang/ast/function_entity_ast.hpp"
#include "rylang/ast/namespace_entity_ast.hpp"
#include "rylang/ast/null_object_ast.hpp"
#include "rylang/ast/variable_entity_ast.hpp"

#endif // RPNX_RYANSCRIPT1031_ENTITY_AST_HEADER
