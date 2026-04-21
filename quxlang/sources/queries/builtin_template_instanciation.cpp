// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/builtin_template_instanciation_spec.hpp>

#include <quxlang/queries/ensig_initialize.hpp>

#include <map>
#include <string>
#include <utility>
#include <vector>

rpnx::querygraph::coroutine< quxlang::builtin_template_instanciation_spec > quxlang::builtin_template_instanciation_impl(initialization_reference input)
{
    if (!typeis< temploid_reference >(input.initializee))
    {
        throw std::logic_error("builtin_template_instanciation called on a non-template selection");
    }

    auto temploid = as< temploid_reference >(input.initializee);
    auto const selected_kind = co_await rpnx::querygraph::request< symbol_type_query >(temploid);
    if (selected_kind != symbol_kind::template_)
    {
        throw std::logic_error("builtin_template_instanciation received a temploid selection that is not a template");
    }

    if (!(co_await rpnx::querygraph::request< template_builtin_query >(temploid)))
    {
        throw std::logic_error("builtin_template_instanciation received a non-builtin template selection");
    }

    auto argument_eval_context = input.context.value_or(context_reference{});
    auto actual_params = input.parameters;

    if (!input.arguments.empty())
    {
        actual_params = {};
        auto split_expression_arguments = [](std::vector< expression_arg > const& args)
            -> std::pair< std::vector< expression_arg const* >, std::map< std::string, expression_arg const* > >
        {
            std::vector< expression_arg const* > positional;
            std::map< std::string, expression_arg const* > named;
            for (auto const& arg : args)
            {
                if (arg.name.has_value())
                {
                    named[*arg.name] = &arg;
                }
                else
                {
                    positional.push_back(&arg);
                }
            }
            return {std::move(positional), std::move(named)};
        };
        auto [positional_expression_args, named_expression_args] = split_expression_arguments(input.arguments);
        auto evaluate_actual =
            [&](argif const& formal, expression_arg const& arg)
        -> rpnx::querygraph::coroutine< builtin_template_instanciation_spec >::cosubroutine< std::optional< parameter_instantiation > >
        {
            constexpr_input_v3 cx_input;
            cx_input.expr = arg.value;
            cx_input.context = argument_eval_context;
            if (formal.requires_static_value)
            {
                cx_input.expected_result_type = formal.type;
                auto eval_result = co_await rpnx::querygraph::request< constexpr_eval_v3_query >(std::move(cx_input));
                auto value_it = eval_result.values.find(constexpr_primary_result_id);
                if (value_it == eval_result.values.end())
                {
                    co_return std::nullopt;
                }
                co_return parameter_value_instantiation{.type = formal.type, .value = value_it->second};
            }

            auto eval_result = co_await rpnx::querygraph::request< constexpr_eval_v3_query >(std::move(cx_input));
            if (!eval_result.type_binding_result.has_value())
            {
                co_return std::nullopt;
            }
            auto canonical_type = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                .context = argument_eval_context,
                .type = *eval_result.type_binding_result,
            });
            if (!canonical_type.has_value())
            {
                co_return std::nullopt;
            }
            co_return parameter_type_instantiation{.type = *canonical_type};
        };

        for (std::size_t i = 0; i < temploid.which.interface.positional.size(); i++)
        {
            if (i >= positional_expression_args.size())
            {
                co_return std::nullopt;
            }
            auto actual = co_await evaluate_actual(temploid.which.interface.positional.at(i), *positional_expression_args.at(i));
            if (!actual.has_value())
            {
                co_return std::nullopt;
            }
            actual_params.positional.push_back(std::move(*actual));
        }

        if (positional_expression_args.size() != temploid.which.interface.positional.size())
        {
            co_return std::nullopt;
        }

        for (auto const& [name, formal] : temploid.which.interface.named)
        {
            auto named_it = named_expression_args.find(name);
            if (named_it == named_expression_args.end())
            {
                co_return std::nullopt;
            }

            auto actual = co_await evaluate_actual(formal, *named_it->second);
            if (!actual.has_value())
            {
                co_return std::nullopt;
            }
            actual_params.named[name] = std::move(*actual);
        }

        if (named_expression_args.size() != temploid.which.interface.named.size())
        {
            co_return std::nullopt;
        }
    }

    auto params = co_await rpnx::querygraph::request< ensig_initialize_query >(ensig_initialization{
        .ensig = temploid.which,
        .params = std::move(actual_params),
        .adaptations = allowed_adaptations::none,
    });
    if (!params.has_value())
    {
        co_return std::nullopt;
    }

    co_return instanciation_reference{
        .temploid = temploid,
        .params = std::move(*params),
    };
}
