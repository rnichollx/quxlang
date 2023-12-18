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

        type_symbol const& chain = this->m_chain;

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

            ast2_module_declaration const& module_ast = module_ast_dp->get();

            set_value(module_ast);
        }
        else if (chain.type() == boost::typeindex::type_id< subentity_reference >())
        {
            auto const& subentity_ref = boost::get< subentity_reference >(chain);
            auto parent_ast_dp = get_dependency(
                [&]
                {
                    return c->lk_type_map(subentity_ref.parent);
                });
            if (!ready())
                return;
            ast2_type_map const& ent = parent_ast_dp->get();

            auto it = ent.globals.find(subentity_ref.subentity_name);

            if (it == ent.globals.end())
            {
                throw std::runtime_error("Failed to look up entity");
            }
            auto res = it->second;

            set_value(res);
        }
        else if (chain.type() == boost::typeindex::type_id< subdotentity_reference >())
        {
            auto const& subentity_ref = boost::get< subdotentity_reference >(chain);
            auto parent_ast_dp = get_dependency(
                [&]
                {
                    return c->lk_type_map(subentity_ref.parent);
                });
            if (!ready())
                return;
            ast2_type_map const& ent = parent_ast_dp->get();

            auto it = ent.members.find(subentity_ref.subdotentity_name);

            if (it == ent.members.end())
            {
                throw std::runtime_error("Failed to look up entity");
            }

            set_value(it->second);
        }
        else
        {
            throw std::invalid_argument("Expected module or subentity reference, got primitive or pointer");
        }
    }
} // namespace rylang
