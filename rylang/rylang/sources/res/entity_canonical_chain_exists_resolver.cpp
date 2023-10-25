//
// Created by Ryan Nicholl on 10/24/23.
//
#include "rylang/res/entity_canonical_chain_exists_resolver.hpp"
#include "rylang/compiler.hpp"

void rylang::entity_canonical_chain_exists_resolver::process(compiler* c)
{
    auto chain = this->m_chain;
    assert(! chain.empty());
    if (chain.size() == 1)
    {
        // TODO: support multiple modules

        auto module_ast_dp = get_dependency(
            [&]
            {
                return c->lk_module_ast("main");
            });

        if (!ready())
            return;

        auto module_ast = module_ast_dp->get();

        auto& root = module_ast.merged_root;


        auto it = root.m_sub_entities.find(chain[0]);
        set_value(it != root.m_sub_entities.end());
        return;
    }
    else
    {
        canonical_lookup_chain parent_chain = chain;
        parent_chain.pop_back();

        auto pre_exists_dp = get_dependency(
            [&]
            {
                return c->lk_entity_canonical_chain_exists(parent_chain);
            });
        if (!ready())
            return;
        bool pre_exists = pre_exists_dp->get();

        if (!pre_exists)
        {
            set_value(false);
            return;
        }

        auto pre_ast_dp = get_dependency(
            [&]
            {
                return c->lk_entity_ast_from_canonical_chain(parent_chain);
            });
        if (!ready())
            return;

        entity_ast pre_ast = pre_ast_dp->get();

        auto it = pre_ast.m_sub_entities.find(chain.back());
        set_value(it != pre_ast.m_sub_entities.end());
    }
}
