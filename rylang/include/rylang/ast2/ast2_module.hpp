//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef AST2_MODULE_HPP
#define AST2_MODULE_HPP
#include <rylang/ast2/ast2_entity.hpp>
#include <vector>

namespace rylang
{
    struct ast2_module
    {
        std::string module_name;
        std::vector< std::pair< std::string, ast2_declaration > > globals;
    };
}

#endif //AST2_MODULE_HPP
