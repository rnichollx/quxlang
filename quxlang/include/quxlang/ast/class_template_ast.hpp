//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef QUXLANG_CLASS_TEMPLATE_AST_HEADER_GUARD
#define QUXLANG_CLASS_TEMPLATE_AST_HEADER_GUARD

#include "class_entity_ast.hpp"
#include "quxlang/data/type_symbol.hpp"

#include <optional>

namespace quxlang
{
    struct class_template_ast
    {
        std::optional<std::int64_t> m_priority;
        std::vector<type_symbol> m_template_args;
        class_entity_ast m_class_entity;
    };
}
#endif //CLASS_TEMPLATE_AST_HEADER_GUARD
