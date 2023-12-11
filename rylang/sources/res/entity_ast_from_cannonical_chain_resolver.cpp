//
// Created by Ryan Nicholl on 9/20/23.
//

#include "rylang/compiler.hpp"

#include "rylang/res/entity_ast_from_canonical_chain_resolver.hpp"

#include "rylang/ast/entity_ast.hpp"

#include "rylang/converters/qual_converters.hpp"
#include "rylang/manipulators/qmanip.hpp"

namespace rylang
{
    void entity_ast_from_canonical_chain_resolver::process(compiler* c)
    {
        // TODO: Get the module

        qualified_symbol_reference const& chain = this->m_chain;

        std::string typestring = to_string(chain);

        if (chain.type() == boost::typeindex::type_id< instanciation_reference >())
        {
            // assert(false);
            //  Don't ask for entity AST of a function parameter set?
            //  TODO: Adjust this later, we should allow this in the future to
            //   support templates.
            //rpnx::unimplemented();
        }
        else if (chain.type() == boost::typeindex::type_id< module_reference >())
        {

            auto module_ast_dp = get_dependency(
                [&]
                {
                    return c->lk_module_ast(boost::get< module_reference >(chain).module_name);
                });

            if (!ready())
                return;

            module_ast const& module_ast = module_ast_dp->get();

            set_value(module_ast.merged_root);
        }
        else if (chain.type() == boost::typeindex::type_id< subentity_reference >())
        {
            auto const& subentity_ref = boost::get< subentity_reference >(chain);

            auto parent_ast_dp = get_dependency(
                [&]
                {
                    return c->lk_entity_ast_from_canonical_chain(subentity_ref.parent);
                });
            if (!ready())
                return;
            entity_ast const& ent = parent_ast_dp->get();

            auto it = ent.m_sub_entities.find(subentity_ref.subentity_name);

            if (it == ent.m_sub_entities.end())
            {
                throw std::runtime_error("Failed to look up entity");
            }

            set_value(it->second.get());
        }
        else if (chain.type() == boost::typeindex::type_id< subdotentity_reference >())
        {

            auto const& subdotentity_ref = boost::get< subdotentity_reference >(chain);

            auto parent_ast_dp = get_dependency(
                [&]
                {
                    return c->lk_entity_ast_from_canonical_chain(subdotentity_ref.parent);
                });
            if (!ready())
                return;
            entity_ast const& ent = parent_ast_dp->get();

            // TODO: Make these lookup differently
            auto it = ent.m_sub_entities.find(subdotentity_ref.subdotentity_name);

            if (it == ent.m_sub_entities.end())
            {
                throw std::runtime_error("Failed to look up entity " + typestring);
            }

            set_value(it->second.get());
        }
        else
        {
            throw std::invalid_argument("Expected module or subentity reference, got primitive or pointer");
        }
    }
} // namespace rylang
