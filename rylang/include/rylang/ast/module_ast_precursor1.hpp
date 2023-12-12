//
// Created by Ryan Nicholl on 8/15/23.
//

#ifndef RYLANG_MODULE_AST_PRECURSOR1_HEADER_GUARD
#define RYLANG_MODULE_AST_PRECURSOR1_HEADER_GUARD

#include <vector>
#include "rylang/ast/file_ast.hpp"

namespace rylang
{
    struct module_ast_precursor1
    {
        std::string module_name;
        std::vector<file_ast> files;
    };
}

#endif // RYLANG_MODULE_AST_PRECURSOR1_HEADER_GUARD
