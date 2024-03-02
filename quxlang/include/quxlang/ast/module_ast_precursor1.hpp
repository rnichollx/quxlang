//
// Created by Ryan Nicholl on 8/15/23.
//

#ifndef QUXLANG_MODULE_AST_PRECURSOR1_HEADER_GUARD
#define QUXLANG_MODULE_AST_PRECURSOR1_HEADER_GUARD

#include "quxlang/ast/file_ast.hpp"
#include <quxlang/ast2/ast2_entity.hpp>
#include <vector>

namespace quxlang
{
    struct module_ast_precursor1
    {
        std::string module_name;
        std::vector<ast2_file_declaration> files;
    };
}

#endif // QUXLANG_MODULE_AST_PRECURSOR1_HEADER_GUARD
