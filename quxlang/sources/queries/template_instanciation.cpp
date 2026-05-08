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
        for (auto const& [name, formal] : temploid.which.interface.named)
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
                auto template_context = type_parent(temploid.templexoid).value_or(context_reference{});
                for (auto const& tmpl : templex.templates)
                {
                    temploid_ensig candidate;
                    candidate.priority = tmpl.priority;

                    bool valid = true;
                    for (auto const& declared_param : tmpl.m_template_args.positional)
                    {
                        auto canonical_param = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                            .context = template_context,
                            .type = declared_param.type,
                        });
                        if (!canonical_param.has_value())
                        {
                            valid = false;
                            break;
                        }

                        candidate.interface.positional.push_back(argif{
                            .type = *canonical_param,
                            .requires_static_value = declared_param.kind == template_parameter_kind::value,
                        });
                    }

                    for (auto const& [api_name, declared_param] : tmpl.m_template_args.named)
                    {
                        auto canonical_param = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                            .context = template_context,
                            .type = declared_param.type,
                        });
                        if (!canonical_param.has_value())
                        {
                            valid = false;
                            break;
                        }

                        auto canonical_name = template_parameter_name(declared_param).value_or(api_name);
                        candidate.interface.named[canonical_name] = argif{
                            .type = *canonical_param,
                            .requires_static_value = declared_param.kind == template_parameter_kind::value,
                        };
                    }

                    if (!valid || candidate != temploid.which)
                    {
                        continue;
                    }

                    for (auto const& [api_name, declared_param] : tmpl.m_template_args.named)
                    {
                        auto canonical_name = template_parameter_name(declared_param).value_or(api_name);
                        auto named_it = named_expression_args.find(api_name);
                        if (named_it != named_expression_args.end())
                        {
                            selected_named_expression_args[canonical_name] = named_it->second;
                        }
                    }
                    break;
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

        if (selected_named_expression_args.size() != temploid.which.interface.named.size())
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
