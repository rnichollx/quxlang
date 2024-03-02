//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef QUXLANG_FUNCTION_AST_RESOLVER_HEADER_GUARD
#define QUXLANG_FUNCTION_AST_RESOLVER_HEADER_GUARD

#include "quxlang/ast/function_ast.hpp"
#include "quxlang/compiler_fwd.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/data/qualified_symbol_reference.hpp"
#include "quxlang/macros.hpp"

namespace quxlang
{
    QUX_CO_RESOLVER(functum_instanciation_ast, type_symbol, ast2_function_declaration);
} // namespace quxlang

#endif // QUXLANG_FUNCTION_AST_RESOLVER_HEADER_GUARD
