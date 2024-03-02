//
// Created by Ryan Nicholl on 9/11/23.
//

#ifndef RYLANG_MODULE_AST_PRECURSOR1_RESOLVER_HEADER_GUARD
#define RYLANG_MODULE_AST_PRECURSOR1_RESOLVER_HEADER_GUARD

#include "rylang/ast/module_ast.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/symbol_id.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "rylang/ast/module_ast_precursor1.hpp"

namespace rylang
{
    QUX_RESOLVER(module_ast_precursor1, std::string, module_ast_precursor1);

} // namespace rylang

#endif // RYLANG_MODULE_AST_PRECURSOR1_RESOLVER_HEADER_GUARD
