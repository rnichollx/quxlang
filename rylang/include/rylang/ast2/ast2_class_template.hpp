//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef AST2_CLASS_TEMPLATE_HEADER_GUARD
#define AST2_CLASS_TEMPLATE_HEADER_GUARD
#include <rylang/data/qualified_symbol_reference.hpp>

namespace rylang
{
    struct ast2_class_template
    {
        std::vector<type_symbol> m_template_args;
    };
}

#endif //AST2_CLASS_TEMPLATE_HEADER_GUARD
