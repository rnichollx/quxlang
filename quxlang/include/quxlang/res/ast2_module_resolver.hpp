// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef AST2_MODULE_RESOLVER_HPP
#define AST2_MODULE_RESOLVER_HPP

#include "quxlang/ast2/ast2_entity.hpp"
#include "quxlang/macros.hpp"

namespace quxlang
{
    QUX_CO_RESOLVER(ast2_module, std::string, ast2_module_declaration);
}

#endif //AST2_MODULE_RESOLVER_HPP
