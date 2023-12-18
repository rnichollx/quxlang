//
// Created by Ryan Nicholl on 10/24/23.
//

#include "rylang/compiler.hpp"

#include "rylang/manipulators/qmanip.hpp"
#include "rylang/res/entity_canonical_chain_exists_resolver.hpp"

void rylang::entity_canonical_chain_exists_resolver::process(compiler* c)
{
    std::cout << this->debug_recursive() << std::endl;
    auto chain = this->m_chain;
    std::string name = to_string(chain);
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

        auto sub_name = boost::get< subdotentity_reference >(chain).subdotentity_name;

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

        std::string sub_name = boost::get< subentity_reference >(chain).subentity_name;


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
        // TODO: support templates
        set_value(false);
    }
    else
    {
        // Other types (e.g. primitives) aren't entities, so we can say that
        // no such *entity* exists, even if a corresponding type does.

        set_value(false);
    }
}
