// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_MODULE_AST_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_MODULE_AST_RESOLVER_HEADER_GUARD

#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/symbol_id.hpp"
#include "rpnx/resolver_utilities.hpp"

#include <quxlang/ast2/ast2_module.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(module_ast, std::string, ast2_module_declaration);
}

#endif // QUXLANG_MODULE_AST_RESOLVER_HEADER_GUARD
