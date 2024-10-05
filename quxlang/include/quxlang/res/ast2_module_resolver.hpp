// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef QUXLANG_RES_AST2_MODULE_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_AST2_MODULE_RESOLVER_HEADER_GUARD

#include "quxlang/ast2/ast2_entity.hpp"
#include "quxlang/macros.hpp"

namespace quxlang
{
    QUX_CO_RESOLVER(ast2_module, std::string, ast2_module_declaration);
}

#endif //AST2_MODULE_RESOLVER_HPP
