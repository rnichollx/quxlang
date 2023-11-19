//
// Created by Ryan Nicholl on 11/4/23.
//
#include "rylang/res/function_overload_selection_resolver.hpp"
#include "rylang/ast/function_ast.hpp"
#include "rylang/compiler.hpp"
#include "rylang/manipulators/qmanip.hpp"

#include <iostream>
#include <sstream>

void rylang::function_overload_selection_resolver::process(compiler* c)
{

    std::cout << "Function overload selection resolver called for " << to_string(m_function_location) << std::endl;
    std::cout << "With args: " << std::endl;
    for (auto const& arg : m_args.argument_types)
    {
        std::cout << "    " << to_string(arg) << std::endl;
    }
    std::size_t eligible_overloads = 0;

    std::optional< call_parameter_information > output_overload;

    call_parameter_information const& call_args = m_args;

    // TODO: Remove this
    std::stringstream ss;
    ss << "Function overload selection resolver called, with args: " << to_string(call_args) << std::endl;

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
        call_parameter_information overload_args;

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

            qualified_symbol_reference arg_type = canonical_type_dp->get();

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

        ss << "Checking overload " << to_string(overload_args) << " callable with " << to_string(call_args) << "? " << std::boolalpha << is_callable << std::endl;

        if (is_callable)
        {
            eligible_overloads++;
            output_overload = overload_args;
        }
    }

    // TODO: Remove this
    std::cout << ss.str() << std::endl;

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
