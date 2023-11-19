//
// Created by Ryan Nicholl on 11/18/23.
//
#include "rylang/res/list_functum_overloads_resolver.hpp"
#include "rylang/compiler.hpp"

void rylang::list_functum_overloads_resolver::process(compiler* c)
{
    auto exists_dp = get_dependency(
        [&]
        {
            return c->lk_entity_canonical_chain_exists(m_functum);
        });

    if (!ready())
    {
        return;
    }

    bool exists = exists_dp->get();

    if (!exists)
    {
        set_value(std::nullopt);
        return;
    }

    auto functum_dp = get_dependency(
        [&]
        {
            return c->lk_entity_ast_from_canonical_chain(m_functum);
        });

    if (!ready())
    {
        return;
    }

    entity_ast functum = functum_dp->get();

    if (!std::holds_alternative< functum_entity_ast >(functum.m_specialization.get()))
    {
        set_value(std::nullopt);
        return;
    }

    functum_entity_ast const& functum_e = std::get< functum_entity_ast >(functum.m_specialization.get());

    std::vector< call_parameter_information > result;
    for (function_ast const& f : functum_e.m_function_overloads)
    {
        call_parameter_information info;
        for (auto const& arg : f.args)
        {
            contextual_type_reference ctx_type;
            ctx_type.context = m_functum;
            ctx_type.type = arg.type;
            auto type_dp = get_dependency(
                [&]
                {
                    return c->lk_canonical_type_from_contextual_type(ctx_type);
                });
            if (!ready())
            {
                return;
            }
            info.argument_types.push_back(type_dp->get());
        }

        result.push_back(info);
    }

    set_value(result);
}
