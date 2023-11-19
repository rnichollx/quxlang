//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_FILE_AST_HEADER
#define RPNX_RYANSCRIPT1031_FILE_AST_HEADER

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

#endif // RPNX_RYANSCRIPT1031_FILE_AST_HEADER
