// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/template_instanciation_spec.hpp>

#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/builtin_template_instanciation.hpp>
#include <quxlang/queries/ensig_initialize.hpp>

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

rpnx::querygraph::coroutine< quxlang::template_instanciation_spec > quxlang::template_instanciation_impl(initialization_reference input)
{
    if (!typeis< temploid_reference >(input.initializee))
    {
        throw quxlang::compiler_bug("template_instanciation called on a non-template selection");
    }

    auto temploid = as< temploid_reference >(input.initializee);

    auto temploid_kind = co_await rpnx::querygraph::request< symbol_type_query >(temploid);
    if (temploid_kind != symbol_kind::template_)
    {
        throw quxlang::compiler_bug("template_instanciation called on a non-template selection");
    }

    if (co_await rpnx::querygraph::request< template_builtin_query >(temploid))
    {
        co_return co_await rpnx::querygraph::request< builtin_template_instanciation_query >(std::move(input));
    }

    auto argument_eval_context = input.context.value_or(type_parent(temploid.templexoid).value_or(context_reference{}));
    auto actual_params = input.parameters;
    auto formal_ensig = co_await rpnx::querygraph::request< temploid_formal_ensig_query >(temploid);
    if (!formal_ensig.has_value())
    {
        throw quxlang::compiler_bug("template_instanciation received an overload reference without a formal ensig");
    }

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
        std::map< std::string, expression_arg const* > selected_named_expression_args;
        for (auto const& [name, formal] : formal_ensig->interface.named)
        {
            auto named_it = named_expression_args.find(name);
            if (named_it != named_expression_args.end())
            {
                selected_named_expression_args[name] = named_it->second;
            }
        }

        if (selected_named_expression_args.size() != named_expression_args.size())
        {
            auto template_parameter_name = [](declared_parameter const& param) -> std::optional< std::string >
            {
                if (param.name.has_value())
                {
                    return param.name;
                }
                if (param.api_name.has_value())
                {
                    return param.api_name;
                }

                auto const& param_type = param.type;
        if (typeis< auto_temploidic >(param_type))
        {
            auto const& name = as< auto_temploidic >(param_type).name;
            if (!name.empty())
            {
                return name;
            }
        }
        else if (typeis< decay_temploidic >(param_type))
        {
            auto const& name = as< decay_temploidic >(param_type).name;
            if (!name.empty())
            {
                return name;
            }
        }
        else if (typeis< type_temploidic >(param_type))
        {
            auto const& name = as< type_temploidic >(param_type).name;
                    if (!name.empty())
                    {
                        return name;
                    }
                }

                return std::nullopt;
            };

            auto const& sym = co_await rpnx::querygraph::request< symboid_query >(temploid.templexoid);
            if (typeis< ast2_templex >(sym))
            {
                auto const& templex = as< ast2_templex >(sym);
                std::uint64_t template_index = temploid.overload_id.value_or(0);
                if (!temploid.overload_id.has_value() && templex.templates.size() != 1)
                {
                    throw compiler_bug("template_instanciation could not resolve a unique template declaration");
                }
                if (template_index >= templex.templates.size())
                {
                    throw compiler_bug("template_instanciation overload id is out of range for the selected templex");
                }

                auto const& tmpl = templex.templates.at(static_cast< std::vector< ast2_template_declaration >::size_type >(template_index));
                for (auto const& [api_name, declared_param] : tmpl.m_template_args.named)
                {
                    auto canonical_name = template_parameter_name(declared_param).value_or(api_name);
                    auto named_it = named_expression_args.find(api_name);
                    if (named_it != named_expression_args.end())
                    {
                        selected_named_expression_args[canonical_name] = named_it->second;
                    }
                }
            }
        }

        auto evaluate_actual =
            [&](argif const& formal, expression_arg const& arg)
        -> rpnx::querygraph::coroutine< template_instanciation_spec >::cosubroutine< std::optional< parameter_instantiation > >
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

        for (std::size_t i = 0; i < formal_ensig->interface.positional.size(); i++)
        {
            if (i >= positional_expression_args.size())
            {
                co_return std::nullopt;
            }
            auto actual = co_await evaluate_actual(formal_ensig->interface.positional.at(i), *positional_expression_args.at(i));
            if (!actual.has_value())
            {
                co_return std::nullopt;
            }
            actual_params.positional.push_back(std::move(*actual));
        }

        if (positional_expression_args.size() != formal_ensig->interface.positional.size())
        {
            co_return std::nullopt;
        }

        for (auto const& [name, formal] : formal_ensig->interface.named)
        {
            auto named_it = selected_named_expression_args.find(name);
            if (named_it == selected_named_expression_args.end())
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

        if (selected_named_expression_args.size() != formal_ensig->interface.named.size())
        {
            co_return std::nullopt;
        }
    }

    type_symbol type_of_this = void_type{};
    if (typeis< submember >(temploid.templexoid))
    {
        type_of_this = as< submember >(temploid.templexoid).of;
    }

    auto params = co_await rpnx::querygraph::request< ensig_initialize_query >(ensig_initialization{
        .ensig = *formal_ensig,
        .params = std::move(actual_params),
        .adaptations = allowed_adaptations::none,
        .type_of_this = type_of_this,
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
