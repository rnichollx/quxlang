//
// Created by Ryan Nicholl on 9/11/23.
//

#ifndef QUXLANG_MODULE_AST_PRECURSOR1_RESOLVER_HEADER_GUARD
#define QUXLANG_MODULE_AST_PRECURSOR1_RESOLVER_HEADER_GUARD

#include "quxlang/ast/module_ast.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/symbol_id.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/ast/module_ast_precursor1.hpp"

namespace quxlang
{
    QUX_RESOLVER(module_ast_precursor1, std::string, module_ast_precursor1);

} // namespace quxlang

#endif // QUXLANG_MODULE_AST_PRECURSOR1_RESOLVER_HEADER_GUARD
