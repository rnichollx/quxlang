//
// Created by Ryan Nicholl on 8/16/23.
//

#ifndef RYLANG_MODULE_AST_HEADER_GUARD
#define RYLANG_MODULE_AST_HEADER_GUARD

#include "rylang/ast/entity_ast.hpp"
#include <map>
#include <string>

namespace rylang
{
    struct module_ast
    {
        std::string module_name;
        entity_ast merged_root;
    };
} // namespace rylang

#endif // RYLANG_MODULE_AST_HEADER_GUARD
