//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef RYLANG_CLASS_TEMPLATE_AST_HEADER_GUARD
#define RYLANG_CLASS_TEMPLATE_AST_HEADER_GUARD

#include "class_entity_ast.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

#include <optional>

namespace rylang
{
    struct class_template_ast
    {
        std::optional<std::int64_t> m_priority;
        std::vector<type_symbol> m_template_args;
        class_entity_ast m_class_entity;
    };
}
#endif //CLASS_TEMPLATE_AST_HEADER_GUARD
