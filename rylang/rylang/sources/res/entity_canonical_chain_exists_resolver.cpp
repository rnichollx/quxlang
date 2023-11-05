//
// Created by Ryan Nicholl on 10/24/23.
//
#include "rylang/res/entity_canonical_chain_exists_resolver.hpp"
#include "rylang/compiler.hpp"
#include "rylang/manipulators/qualified_reference.hpp"

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
    else if (chain.type() == boost::typeindex::type_id< parameter_set_reference >())
    {
        parameter_set_reference const& param_set = boost::get< parameter_set_reference >(chain);
        // We only care if the parent exists, we can exist but be an invalid functaniod otherwise?
        // Maybe this resolver should be renamed...

        assert(param_set.callee.type() != boost::typeindex::type_id< context_reference >());

        // TODO: Distingiush between the functum/template existing but not the particular
        //  functanoid/temtanoid existing, and the functum/template not existing at all.
        //  Currently it's not possible for this distinction to matter, since it is impossible
        //  for a contextual type to be converted to a parameter set with the callee being
        //  a context_reference, due to the way the collector parses conxtual qualified
        //  references into the abstract syntax tree from the input text, thus the only
        //  case in which this would be significant is where a template is used which
        //  isn't implemented yet.
        auto parent_exists_dp = get_dependency(
            [&]
            {
                return c->lk_entity_canonical_chain_exists(param_set.callee);
            });

        if (!ready())
        {
            return;
        }

        bool parent_exists = parent_exists_dp->get();

        set_value(parent_exists);
        return;
    }
    else
    {
        // Other types (e.g. primitives) aren't entities, so we can say that
        // no such *entity* exists, even if a corresponding type does.

        set_value(false);
    }
}
