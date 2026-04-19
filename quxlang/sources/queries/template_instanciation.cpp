// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/template_instanciation_spec.hpp>
#include <quxlang/manipulators/typeutils.hpp>

namespace quxlang
{
    struct template_instanciation_candidate
    {
        temploid_reference temploid;
        instatype params;
    };

    auto template_parameter_name(type_symbol const& param) -> std::optional< std::string >
    {
        if (typeis< auto_temploidic >(param))
        {
            auto const& name = as< auto_temploidic >(param).name;
            if (!name.empty())
            {
                return name;
            }
        }
        else if (typeis< type_temploidic >(param))
        {
            auto const& name = as< type_temploidic >(param).name;
            if (!name.empty())
            {
                return name;
            }
        }

        return std::nullopt;
    }

    auto template_parameter_name(declared_parameter const& param) -> std::optional< std::string >
    {
        if (param.name.has_value())
        {
            return param.name;
        }
        if (param.api_name.has_value())
        {
            return param.api_name;
        }

        return template_parameter_name(param.type);
    }

    auto template_expression_arguments(std::vector< expression_arg > const& args) -> std::pair< std::vector< expression_arg const* >, std::map< std::string, expression_arg const* > >
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
    }
}


rpnx::querygraph::coroutine< quxlang::template_instanciation_spec > quxlang::template_instanciation_impl(initialization_reference input)
{
    type_symbol initializee = input.initializee;

    if (typeis< temploid_reference >(initializee))
    {
        auto const& temploid = as< temploid_reference >(initializee);

        auto temploid_kind = co_await rpnx::querygraph::request< symbol_type_query >(temploid);
        if (temploid_kind != symbol_kind::template_)
        {
            throw std::logic_error("template_instanciation called on a non-template selection");
        }

        co_return instanciation_reference{
            .temploid = temploid,
            .params = input.parameters,
        };
    }

    auto initializee_kind = co_await rpnx::querygraph::request< symbol_type_query >(initializee);
    if (initializee_kind != symbol_kind::templex)
    {
        throw std::logic_error("template_instanciation called on a non-templex");
    }

    auto const& sym = co_await rpnx::querygraph::request< symboid_query >(initializee);
    if (!typeis< ast2_templex >(sym))
    {
        throw compiler_bug("templex symbol did not resolve to ast2_templex");
    }

    auto const& templex = as< ast2_templex >(sym);
    auto template_context = type_parent(initializee).value_or(context_reference{});
    auto argument_eval_context = input.context.value_or(template_context);
    auto const use_expression_arguments = !input.arguments.empty();
    auto [positional_expression_args, named_expression_args] = template_expression_arguments(input.arguments);

    std::vector< template_instanciation_candidate > matches;
    std::optional< std::int64_t > highest_priority;

    for (auto const& tmpl : templex.templates)
    {
        auto const declared_positional_count = tmpl.m_template_args.positional.size();
        auto const declared_named_count = tmpl.m_template_args.named.size();
        auto const input_positional_count = use_expression_arguments ? positional_expression_args.size() : input.parameters.positional.size();
        auto const input_named_count = use_expression_arguments ? named_expression_args.size() : input.parameters.named.size();
        if (declared_positional_count + declared_named_count != input_positional_count + input_named_count)
        {
            continue;
        }

        temploid_reference candidate;
        candidate.templexoid = initializee;
        candidate.which.priority = tmpl.priority;
        instatype candidate_params;

        bool matched = true;
        auto evaluate_actual = [&](declared_parameter const& declared_param, type_symbol const& arg_pattern, expression_arg const& arg) -> rpnx::querygraph::coroutine< template_instanciation_spec >::cosubroutine< std::optional< parameter_instantiation > >
        {
            constexpr_input_v3 cx_input;
            cx_input.expr = arg.value;
            cx_input.context = argument_eval_context;
            if (declared_param.kind == template_parameter_kind::value)
            {
                cx_input.expected_result_type = arg_pattern;
                auto eval_result = co_await rpnx::querygraph::request< constexpr_eval_v3_query >(std::move(cx_input));
                auto value_it = eval_result.values.find(constexpr_primary_result_id);
                if (value_it == eval_result.values.end())
                {
                    co_return std::nullopt;
                }
                co_return parameter_value_instantiation{.type = arg_pattern, .value = value_it->second};
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

        auto match_actual = [&](declared_parameter const& declared_param, type_symbol const& arg_pattern, parameter_instantiation const& actual) -> bool
        {
            if (declared_param.kind == template_parameter_kind::value)
            {
                if (!actual.template type_is< parameter_value_instantiation >())
                {
                    return false;
                }
            }
            else if (!actual.template type_is< parameter_type_instantiation >())
            {
                return false;
            }

            return match_template(arg_pattern, parameter_instantiation_type(actual)).has_value();
        };

        for (std::size_t i = 0; i < tmpl.m_template_args.positional.size(); i++)
        {
            auto const& declared_param = tmpl.m_template_args.positional.at(i);
            auto arg_pattern_opt = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                .context = template_context,
                .type = declared_param.type,
            });

            if (!arg_pattern_opt.has_value())
            {
                matched = false;
                break;
            }

            auto const& arg_pattern = *arg_pattern_opt;
            std::optional< parameter_instantiation > actual;
            if (use_expression_arguments)
            {
                if (i >= positional_expression_args.size())
                {
                    matched = false;
                    break;
                }
                actual = co_await evaluate_actual(declared_param, arg_pattern, *positional_expression_args.at(i));
            }
            else if (i < input.parameters.positional.size())
            {
                actual = input.parameters.positional.at(i);
            }

            if (!actual.has_value())
            {
                matched = false;
                break;
            }

            if (!match_actual(declared_param, arg_pattern, *actual))
            {
                matched = false;
                break;
            }

            candidate.which.interface.positional.push_back(argif{.type = arg_pattern, .requires_static_value = declared_param.kind == template_parameter_kind::value});
            candidate_params.positional.push_back(std::move(*actual));
        }

        if (!matched || input_positional_count != tmpl.m_template_args.positional.size())
        {
            continue;
        }

        for (auto const& [name, declared_param] : tmpl.m_template_args.named)
        {
            auto arg_pattern_opt = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                .context = template_context,
                .type = declared_param.type,
            });

            if (!arg_pattern_opt.has_value())
            {
                matched = false;
                break;
            }

            auto const& arg_pattern = *arg_pattern_opt;
            std::optional< parameter_instantiation > actual;
            if (use_expression_arguments)
            {
                auto named_it = named_expression_args.find(name);
                if (named_it == named_expression_args.end())
                {
                    matched = false;
                    break;
                }
                actual = co_await evaluate_actual(declared_param, arg_pattern, *named_it->second);
            }
            else
            {
                auto named_it = input.parameters.named.find(name);
                if (named_it == input.parameters.named.end())
                {
                    matched = false;
                    break;
                }
                actual = named_it->second;
            }

            if (!actual.has_value() || !match_actual(declared_param, arg_pattern, *actual))
            {
                matched = false;
                break;
            }

            auto canonical_name = template_parameter_name(declared_param).value_or(name);
            candidate.which.interface.named[canonical_name] = argif{.type = arg_pattern, .requires_static_value = declared_param.kind == template_parameter_kind::value};
            candidate_params.named[canonical_name] = std::move(*actual);
        }

        if (!matched || input_named_count != tmpl.m_template_args.named.size())
        {
            continue;
        }

        auto priority = tmpl.priority.value_or(0);
        if (!highest_priority.has_value() || priority > *highest_priority)
        {
            highest_priority = priority;
            matches.clear();
            matches.push_back(template_instanciation_candidate{.temploid = std::move(candidate), .params = std::move(candidate_params)});
        }
        else if (priority == *highest_priority)
        {
            matches.push_back(template_instanciation_candidate{.temploid = std::move(candidate), .params = std::move(candidate_params)});
        }
    }

    if (matches.empty())
    {
        QUX_WHY("No matching template found");
        co_return std::nullopt;
    }

    if (matches.size() > 1)
    {
        throw std::logic_error("Ambiguous template instanciation");
    }

    co_return instanciation_reference{
        .temploid = matches.front().temploid,
        .params = matches.front().params,
    };
}
