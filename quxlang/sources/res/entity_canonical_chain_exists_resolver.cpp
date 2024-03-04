//
// Created by Ryan Nicholl on 10/24/23.
//

#include "quxlang/compiler.hpp"

#include "quxlang/manipulators/qmanip.hpp"
#include "quxlang/res/entity_canonical_chain_exists_resolver.hpp"

void quxlang::entity_canonical_chain_exists_resolver::process(compiler* c)
{
    //std::cout << this->debug_recursive() << std::endl;
    auto chain = this->m_chain;
    std::string name = to_string(chain);
    assert(!qualified_is_contextual(chain));
    if (chain.type() == boost::typeindex::type_id< module_reference >())
    {
        std::string module_name = as< module_reference >(chain).module_name;
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

        auto sub_name = as< subdotentity_reference >(chain).subdotentity_name;

        if (!typeis< ast2_class_declaration >(parent_ast))
        {
            set_value(false);
            return;
        }

        auto class_map_dp = get_dependency(
            [&]
            {
                return c->lk_type_map(parent.value());
            });
        if (!ready())
        {
            return;
        }
        auto const & class_map = class_map_dp->get();

        if (class_map.members.find(sub_name) == class_map.members.end())
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

        auto class_map_dp = get_dependency(
            [&]
            {
                return c->lk_type_map(parent.value());
            });
        if (!ready())
            return;

        if (name == "[[module: main]]::quz::bif")
        {
            int x = 1;
        }
        auto const & class_map = class_map_dp->get();

        std::string sub_name = as< subentity_reference >(chain).subentity_name;


        if (class_map.globals.find(sub_name) == class_map.globals.end())
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

        instanciation_reference const & ref = as< instanciation_reference >(chain);

        // TODO: Check if this set of arguments matches the temploid


        auto exists_dep = c->lk_entity_canonical_chain_exists(ref.callee);

        if (!ready())
        {
            return;
        }

        auto exists = exists_dep->get();






        set_value(exists);
    }
    else
    {
        // Other types (e.g. primitives) aren't entities, so we can say that
        // no such *entity* exists, even if a corresponding type does.

        set_value(false);
    }
}
