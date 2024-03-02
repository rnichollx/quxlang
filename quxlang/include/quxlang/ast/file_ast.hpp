//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef QUXLANG_FILE_AST_HEADER_GUARD
#define QUXLANG_FILE_AST_HEADER_GUARD

#include <string>
#include <map>
#include "quxlang/ast/entity_ast.hpp"

namespace quxlang
{
    struct file_ast
    {
        std::string filename;
        std::string module_name;
        std::map<std::string, std::string> imports;
        entity_ast root;
    };

} // namespace quxlang

#endif // QUXLANG_FILE_AST_HEADER_GUARD
