//
// Created by Ryan Nicholl on 10/24/23.
//

#include "rylang/compiler.hpp"


#include "rylang/res/entity_canonical_chain_exists_resolver.hpp"
#include "rylang/manipulators/qmanip.hpp"

void rylang::entity_canonical_chain_exists_resolver::process(compiler* c)
{
    auto chain = this->m_chain;
    assert(!qualified_is_contextual(chain));
    if (chain.type() == boost::typeindex::type_id< module_reference >())
    {
        std::string module_name = boost::get< module_reference >(chain).module_name;
        // TODO: Check if module exists
        auto module_ast_dp = get_dependency(
            [&]
            {
                return c->lk_module_ast(module_name);
            });
        if (!ready())
            return;

        auto module_ast = module_ast_dp->get();
        set_value(true);
        return;
    }
    else if (chain.type() == boost::typeindex::type_id< subdotentity_reference >())
    {
        auto parent = qualified_parent(chain);
        assert(parent.has_value());

        auto parent_exists_dp = get_dependency(
            [&]
            {
                return c->lk_entity_canonical_chain_exists(parent.value());
            });

        if (!ready())
            return;

        bool parent_exists = parent_exists_dp->get();

        if (!parent_exists)
        {
            set_value(false);
            return;
        }

        auto parent_ast_dp = get_dependency(
            [&]
            {
                return c->lk_entity_ast_from_canonical_chain(parent.value());
            });
        if (!ready())
            return;

        auto parent_ast = parent_ast_dp->get();

        if (parent_ast.m_sub_entities.find(boost::get< subdotentity_reference >(chain).subdotentity_name) == parent_ast.m_sub_entities.end())
        {
            set_value(false);
            return;
        }
        else
        {
            set_value(true);
            return;
        }
    }
    else if (chain.type() == boost::typeindex::type_id< subentity_reference >())
    {
        auto parent = qualified_parent(chain);
        assert(parent.has_value());

        auto parent_exists_dp = get_dependency(
            [&]
            {
                return c->lk_entity_canonical_chain_exists(parent.value());
            });

        if (!ready())
            return;

        bool parent_exists = parent_exists_dp->get();

        if (!parent_exists)
        {
            set_value(false);
            return;
        }

        auto parent_ast_dp = get_dependency(
            [&]
            {
                return c->lk_entity_ast_from_canonical_chain(parent.value());
            });
        if (!ready())
            return;

        auto parent_ast = parent_ast_dp->get();

        if (parent_ast.m_sub_entities.find(boost::get< subentity_reference >(chain).subentity_name) == parent_ast.m_sub_entities.end())
        {
            set_value(false);
            return;
        }
        else
        {
            set_value(true);
            return;
        }
    }
    else if (chain.type() == boost::typeindex::type_id< instanciation_reference >())
    {
        set_value(false);
    }
    else
    {
        // Other types (e.g. primitives) aren't entities, so we can say that
        // no such *entity* exists, even if a corresponding type does.

        set_value(false);
    }
}
