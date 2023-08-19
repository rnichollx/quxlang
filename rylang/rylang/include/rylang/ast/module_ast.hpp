//
// Created by Ryan Nicholl on 8/16/23.
//

#ifndef RPNX_RYANSCRIPT1031_MODULE_AST_HEADER
#define RPNX_RYANSCRIPT1031_MODULE_AST_HEADER

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

#endif // RPNX_RYANSCRIPT1031_MODULE_AST_HEADER
