//
// Created by Ryan Nicholl on 9/20/23.
//

#include "rylang/compiler.hpp"
#include "rylang/res/entity_ast_from_chain_resolver.hpp"

void rylang::entity_ast_from_chain_resolver::process(compiler* c)
{
    auto canonical_chain_dep = get_dependency(
        [&]
        {
            return c->lk_canonical_chain(m_chain, m_context);
        });

    if (!ready())
        return;

    canonical_lookup_chain canonical_chain = canonical_chain_dep->get();

    auto ast_dep = get_dependency(
        [&]
        {
            return c->lk_entity_ast_from_canonical_chain(canonical_chain);
        });

    if (!ready())
        return;

    set_value(ast_dep->get());
}