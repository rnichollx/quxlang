//
// Created by Ryan Nicholl on 9/20/23.
//

#include "rylang/res/entity_ast_from_canonical_chain_resolver.hpp"

#include "rylang/compiler.hpp"
#include "rylang/ast/entity_ast.hpp"


namespace rylang
{
    void entity_ast_from_canonical_chain_resolver::process(compiler* c)
    {
        // TODO: Get the module

        auto module = "main";

        auto module_ast_dep = get_dependency([&]{
            return c->lk_module_ast(module);
        });
    }
} // namespace rylang
