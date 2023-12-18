//
// Created by Ryan Nicholl on 8/15/23.
//

#ifndef RYLANG_MODULE_AST_PRECURSOR1_HEADER_GUARD
#define RYLANG_MODULE_AST_PRECURSOR1_HEADER_GUARD

#include "rylang/ast/file_ast.hpp"
#include <rylang/ast2/ast2_entity.hpp>
#include <vector>

namespace rylang
{
    struct module_ast_precursor1
    {
        std::string module_name;
        std::vector<ast2_file_declaration> files;
    };
}

#endif // RYLANG_MODULE_AST_PRECURSOR1_HEADER_GUARD
