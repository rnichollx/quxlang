// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/templex_select_template_spec.hpp>

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/manipulators/typeutils.hpp>

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

rpnx::querygraph::coroutine< quxlang::templex_select_template_spec > quxlang::templex_select_template_impl(initialization_reference input)
{
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

    if (typeis< temploid_reference >(input.initializee))
    {
        auto const& selected = as< temploid_reference >(input.initializee);
        auto const selected_kind = co_await rpnx::querygraph::request< symbol_type_query >(selected);

        if (selected_kind != symbol_kind::template_)
        {
            throw quxlang::compiler_bug("templex_select_template received a temploid selection that is not a template.");
        }

        co_return selected;
    }

    auto initializee_kind = co_await rpnx::querygraph::request< symbol_type_query >(input.initializee);
    if (initializee_kind != symbol_kind::templex)
    {
        co_return std::nullopt;
    }

    auto builtin_templates = co_await rpnx::querygraph::request< templex_builtins_query >(input.initializee);
    if (!builtin_templates.empty())
    {
        std::vector< temploid_reference > matches;
        std::optional< std::int64_t > highest_priority;
        auto argument_eval_context = input.context.value_or(context_reference{});
        auto const use_expression_arguments = !input.arguments.empty();
        auto [positional_expression_args, named_expression_args] = split_expression_arguments(input.arguments);

        for (std::size_t builtin_template_index = 0; builtin_template_index < builtin_templates.size(); builtin_template_index++)
        {
            auto const& builtin_template = builtin_templates.at(builtin_template_index);
            auto const declared_positional_count = builtin_template.template_args.positional.size();
            auto const declared_named_count = builtin_template.template_args.named.size();
            auto const input_positional_count = use_expression_arguments ? positional_expression_args.size() : input.parameters.positional.size();
            auto const input_named_count = use_expression_arguments ? named_expression_args.size() : input.parameters.named.size();
            if (declared_positional_count + declared_named_count != input_positional_count + input_named_count)
            {
                continue;
            }

            temploid_reference candidate;
            candidate.templexoid = input.initializee;
            candidate.overload_id = static_cast< std::uint64_t >(builtin_template_index);

            bool matched = true;
            auto evaluate_actual =
                [&](declared_parameter const& declared_param, type_symbol const& arg_pattern, expression_arg const& arg)
                -> rpnx::querygraph::coroutine< templex_select_template_spec >::cosubroutine< std::optional< parameter_instantiation > >
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

            for (std::size_t i = 0; i < builtin_template.template_args.positional.size(); i++)
            {
                auto const& declared_param = builtin_template.template_args.positional.at(i);
                std::optional< type_symbol > arg_pattern_opt = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                    .context = argument_eval_context,
                    .type = declared_param.type,
                });
                if (!arg_pattern_opt.has_value())
                {
                    matched = false;
                    break;
                }

                type_symbol const& arg_pattern = *arg_pattern_opt;
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

                if (!actual.has_value() || !match_actual(declared_param, arg_pattern, *actual))
                {
                    matched = false;
                    break;
                }
            }

            if (!matched || input_positional_count != builtin_template.template_args.positional.size())
            {
                continue;
            }

            for (auto const& [name, declared_param] : builtin_template.template_args.named)
            {
                std::optional< type_symbol > arg_pattern_opt = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                    .context = argument_eval_context,
                    .type = declared_param.type,
                });
                if (!arg_pattern_opt.has_value())
                {
                    matched = false;
                    break;
                }

                type_symbol const& arg_pattern = *arg_pattern_opt;
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
            }

            if (!matched || input_named_count != builtin_template.template_args.named.size())
            {
                continue;
            }

            auto const priority = builtin_template.priority.value_or(0);
            if (!highest_priority.has_value() || priority > *highest_priority)
            {
                highest_priority = priority;
                matches.clear();
                matches.push_back(std::move(candidate));
            }
            else if (priority == *highest_priority)
            {
                matches.push_back(std::move(candidate));
            }
        }

        if (matches.empty())
        {
            QUX_WHY("No matching template found");
            co_return std::nullopt;
        }

        if (matches.size() > 1)
        {
            throw quxlang::semantic_compilation_error("Ambiguous template instanciation");
        }

        co_return matches.front();
    }

    auto const& sym = co_await rpnx::querygraph::request< symboid_query >(input.initializee);
    if (!typeis< ast2_templex >(sym))
    {
        throw compiler_bug("templex symbol did not resolve to ast2_templex");
    }

    auto const& templex = as< ast2_templex >(sym);
    auto template_context = type_parent(input.initializee).value_or(context_reference{});
    auto argument_eval_context = input.context.value_or(template_context);
    auto const use_expression_arguments = !input.arguments.empty();
    auto [positional_expression_args, named_expression_args] = split_expression_arguments(input.arguments);

    std::vector< temploid_reference > matches;
    std::optional< std::int64_t > highest_priority;

    for (std::size_t template_index = 0; template_index < templex.templates.size(); template_index++)
    {
        auto const& tmpl = templex.templates.at(template_index);
        auto const declared_positional_count = tmpl.m_template_args.positional.size();
        auto const declared_named_count = tmpl.m_template_args.named.size();
        auto const input_positional_count = use_expression_arguments ? positional_expression_args.size() : input.parameters.positional.size();
        auto const input_named_count = use_expression_arguments ? named_expression_args.size() : input.parameters.named.size();
        if (declared_positional_count + declared_named_count != input_positional_count + input_named_count)
        {
            continue;
        }

        temploid_reference candidate;
        candidate.templexoid = input.initializee;
        candidate.overload_id = static_cast< std::uint64_t >(template_index);

        bool matched = true;
        auto evaluate_actual =
            [&](declared_parameter const& declared_param, type_symbol const& arg_pattern, expression_arg const& arg)
            -> rpnx::querygraph::coroutine< templex_select_template_spec >::cosubroutine< std::optional< parameter_instantiation > >
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
        }

        if (!matched || input_named_count != tmpl.m_template_args.named.size())
        {
            continue;
        }

        auto const priority = tmpl.priority.value_or(0);
        if (!highest_priority.has_value() || priority > *highest_priority)
        {
            highest_priority = priority;
            matches.clear();
            matches.push_back(std::move(candidate));
        }
        else if (priority == *highest_priority)
        {
            matches.push_back(std::move(candidate));
        }
    }

    if (matches.empty())
    {
        QUX_WHY("No matching template found");
        co_return std::nullopt;
    }

    if (matches.size() > 1)
    {
        throw quxlang::semantic_compilation_error("Ambiguous template instanciation");
    }

    co_return matches.front();
}
