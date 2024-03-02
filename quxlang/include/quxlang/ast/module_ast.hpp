//
// Created by Ryan Nicholl on 8/16/23.
//

#ifndef QUXLANG_MODULE_AST_HEADER_GUARD
#define QUXLANG_MODULE_AST_HEADER_GUARD

#include "quxlang/ast/entity_ast.hpp"
#include <map>
#include <string>

namespace quxlang
{
    struct module_ast
    {
        std::string module_name;
        entity_ast merged_root;
    };
} // namespace quxlang

#endif // QUXLANG_MODULE_AST_HEADER_GUARD
