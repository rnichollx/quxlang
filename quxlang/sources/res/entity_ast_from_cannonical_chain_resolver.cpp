//
// Created by Ryan Nicholl on 9/20/23.
//

#include "quxlang/compiler.hpp"
#include "quxlang/macros.hpp"

#include "quxlang/res/entity_ast_from_canonical_chain_resolver.hpp"

#include "quxlang/ast/entity_ast.hpp"

#include "quxlang/converters/qual_converters.hpp"
#include "quxlang/manipulators/qmanip.hpp"

namespace quxlang
{
    void entity_ast_from_canonical_chain_resolver::process(compiler* c)
    {
        // TODO: Get the module

        type_symbol const& chain = this->m_chain;

        std::string typestring = to_string(chain);

        if (chain.type() == boost::typeindex::type_id< instanciation_reference >())
        {
            instanciation_reference inst = as<instanciation_reference>(chain);

            // Get the template argument

            QUX_GETDEP_T(ast, temploid_instanciation_ast, (inst), ast2_node)

            // TODO: switch based on functum/template

            if (typeis<ast2_template_declaration>(ast))
            {
                ast2_template_declaration const & t = as<ast2_template_declaration>(ast);

                set_value(t.m_class);
                return;
            }


            assert(typeis<ast2_function_declaration>(ast));
            set_value(ast);
            return;
        }
        else if (chain.type() == boost::typeindex::type_id< module_reference >())
        {

            auto module_ast_dp = get_dependency(
                [&]
                {
                    return c->lk_module_ast(as< module_reference >(chain).module_name);
                });

            if (!ready())
                return;

            ast2_module_declaration const& module_ast = module_ast_dp->get();

            set_value(module_ast);
        }
        else if (chain.type() == boost::typeindex::type_id< subentity_reference >())
        {
            auto const& subentity_ref = as< subentity_reference >(chain);
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
            auto const& subentity_ref = as< subdotentity_reference >(chain);
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
            return;
        }
        else
        {
            throw std::invalid_argument("Expected module or subentity reference, got primitive or pointer");
        }
    }


} // namespace quxlang
