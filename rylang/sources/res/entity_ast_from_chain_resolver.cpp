//
// Created by Ryan Nicholl on 9/20/23.
//

#include "rylang/compiler.hpp"

void rylang::entity_ast_from_chain_resolver::process(compiler* c)
{
    auto canonical_chain_dep = get_dependency(
        [&]
        {
            contextual_type_reference typ;
            typ.context = m_context;
            typ.type = m_chain;
            return c->lk_canonical_type_from_contextual_type(m_chain, m_context);
        });

    if (!ready())
        return;

    qualified_symbol_reference canonical_chain = canonical_chain_dep->get();

    auto ast_dep = get_dependency(
        [&]
        {
            return c->lk_entity_ast_from_canonical_chain(canonical_chain);
        });

    if (!ready())
        return;

    set_value(ast_dep->get());
}