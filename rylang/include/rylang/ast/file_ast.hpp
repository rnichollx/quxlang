//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RYLANG_FILE_AST_HEADER_GUARD
#define RYLANG_FILE_AST_HEADER_GUARD

#include <string>
#include <map>
#include "rylang/ast/entity_ast.hpp"

namespace rylang
{
    struct file_ast
    {
        std::string filename;
        std::string module_name;
        std::map<std::string, std::string> imports;
        entity_ast root;
    };

} // namespace rylang

#endif // RYLANG_FILE_AST_HEADER_GUARD
