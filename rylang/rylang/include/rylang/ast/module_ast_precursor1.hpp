//
// Created by Ryan Nicholl on 8/15/23.
//

#ifndef RPNX_RYANSCRIPT1031_MODULE_AST_PRECURSOR1_HEADER
#define RPNX_RYANSCRIPT1031_MODULE_AST_PRECURSOR1_HEADER

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

#endif // RPNX_RYANSCRIPT1031_MODULE_AST_PRECURSOR1_HEADER
