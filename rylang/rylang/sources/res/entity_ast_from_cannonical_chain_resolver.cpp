//
// Created by Ryan Nicholl on 9/20/23.
//

#include "rylang/res/entity_ast_from_canonical_chain_resolver.hpp"

#include "rylang/ast/entity_ast.hpp"
#include "rylang/compiler.hpp"

namespace rylang
{
    void entity_ast_from_canonical_chain_resolver::process(compiler* c)
    {
        // TODO: Get the module

        canonical_lookup_chain const& chain = this->m_chain;

        if (chain.size() == 0)
        {
            // error
            assert(false);
        }

        if (chain.size() == 1)
        {
            // get module and lookup ast from module
            // TODO: support multiple modules

            auto module_ast_dp = get_dependency(
                [&]
                {
                    return c->lk_module_ast("main");
                });

            if (!ready())
                return;

            module_ast const& module_ast = module_ast_dp->get();

            auto it = module_ast.merged_root.m_sub_entities.find(chain[0]);

            if (it == module_ast.merged_root.m_sub_entities.end())
            {
                throw std::runtime_error("Failed to look up entity");
            }

            set_value(it->second.get());
        }

        else
        {
            // get entity ast from parent entity
            canonical_lookup_chain parent_chain = chain;
            parent_chain.pop_back();

            auto parent_ast_dp = get_dependency(
                [&]
                {
                    return c->lk_entity_ast_from_canonical_chain(parent_chain);
                });

            if (!ready())
                return;

            entity_ast const& parent_ast = parent_ast_dp->get();

            auto it = parent_ast.m_sub_entities.find(chain.back());
            if (it == parent_ast.m_sub_entities.end())
            {
                std::stringstream ss;
                ss << "Failed to look up entity " << chain.back() << " in parent entity ";
                bool first = true;
                for (auto const& elm : parent_chain)
                {
                    if (!first)
                    {
                        ss << "::";
                    }
                    ss << elm;
                    first = false;
                }
                ss << "\n";

                throw std::runtime_error(ss.str());
            }
        }
    }
} // namespace rylang
