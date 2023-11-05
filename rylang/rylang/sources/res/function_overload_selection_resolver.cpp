//
// Created by Ryan Nicholl on 11/4/23.
//
#include "rylang/res/function_overload_selection_resolver.hpp"
#include "rylang/ast/function_ast.hpp"
#include "rylang/compiler.hpp"

void rylang::function_overload_selection_resolver::process(compiler* c)
{
    std::size_t eligible_overloads = 0;

    std::optional< call_overload_set > output_overload;

    call_overload_set const& call_args = m_args;

    // We need to collect the overloads that are callable with the given arguments

    auto entity_ast_dep = get_dependency(
        [&]
        {
            return c->lk_entity_ast_from_canonical_chain(m_function_location);
        });

    if (!ready())
        return;

    entity_ast ent = entity_ast_dep->get();

    if (ent.type() != entity_type::function_type)
    {
        throw std::logic_error("Cannot resolve function overload of non-functum entity");
    }

    functum_entity_ast const& func_ast = ent.get_as< functum_entity_ast >();

    for (function_ast const& overload : func_ast.m_function_overloads)
    {
        call_overload_set overload_args;

        for (auto const& arg : overload.args)
        {
            contextual_type_reference ctx_arg_type;
            ctx_arg_type.type = arg.type;
            ctx_arg_type.context = m_function_location;
            auto canonical_type_dp = get_dependency(
                [&]
                {
                    return c->lk_canonical_type_from_contextual_type(ctx_arg_type);
                });
            if (!ready())
                return;

            canonical_type_reference arg_type = canonical_type_dp->get();

            overload_args.argument_types.push_back(arg_type);
        }

        auto is_callable_dp = get_dependency(
            [&]
            {
                return c->lk_overload_set_is_callable_with(std::make_pair(overload_args, call_args));
            });

        if (!ready())
            return;

        bool is_callable = is_callable_dp->get();

        if (is_callable)
        {
            eligible_overloads++;
            output_overload = overload_args;
        }
    }

    if (eligible_overloads == 0)
    {
        throw std::runtime_error("No eligible overloads");
    }
    else if (eligible_overloads > 1)
    {
        throw std::runtime_error("Ambiguous overload");
    }

    set_value(output_overload.value());
}
