//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RYLANG_FUNCTION_AST_RESOLVER_HEADER_GUARD
#define RYLANG_FUNCTION_AST_RESOLVER_HEADER_GUARD

#include "rylang/ast/function_ast.hpp"
#include "rylang/compiler_fwd.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"
#include "rylang/macros.hpp"

namespace rylang
{
    QUX_CO_RESOLVER(functum_instanciation_ast, ast2_function_declaration, type_symbol);
} // namespace rylang

#endif // RYLANG_FUNCTION_AST_RESOLVER_HEADER_GUARD
