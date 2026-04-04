// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/template_instanciation_spec.hpp>
#include <quxlang/manipulators/typeutils.hpp>

namespace
{
    auto template_parameter_name(quxlang::type_symbol const& param) -> std::optional< std::string >
    {
        using namespace quxlang;

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

    auto template_parameter_name(quxlang::declared_parameter const& param) -> std::optional< std::string >
    {
        if (param.api_name.has_value())
        {
            return param.api_name;
        }

        return template_parameter_name(param.type);
    }
}


rpnx::querygraph::coroutine< quxlang::template_instanciation_spec > quxlang::template_instanciation_impl(initialization_reference input)
{
    type_symbol initializee = input.initializee;

    if (typeis< temploid_reference >(initializee))
    {
        auto const& temploid = as< temploid_reference >(initializee);

        auto temploid_kind = co_await rpnx::querygraph::query_request< symbol_type_query >(temploid);
        if (temploid_kind != symbol_kind::template_)
        {
            throw std::logic_error("template_instanciation called on a non-template selection");
        }

        co_return instanciation_reference{
            .temploid = temploid,
            .params = input.parameters,
        };
    }

    auto initializee_kind = co_await rpnx::querygraph::query_request< symbol_type_query >(initializee);
    if (initializee_kind != symbol_kind::templex)
    {
        throw std::logic_error("template_instanciation called on a non-templex");
    }

    auto const& sym = co_await rpnx::querygraph::query_request< symboid_query >(initializee);
    if (!typeis< ast2_templex >(sym))
    {
        throw compiler_bug("templex symbol did not resolve to ast2_templex");
    }

    auto const& templex = as< ast2_templex >(sym);
    auto template_context = type_parent(initializee).value_or(context_reference{});

    std::vector< temploid_reference > matches;
    std::optional< std::int64_t > highest_priority;

    for (auto const& tmpl : templex.templates)
    {
        if (tmpl.m_template_args.positional.size() + tmpl.m_template_args.named.size() != input.parameters.positional.size() + input.parameters.named.size())
        {
            continue;
        }

        temploid_reference candidate;
        candidate.templexoid = initializee;
        candidate.which.priority = tmpl.priority;

        bool matched = true;
        for (std::size_t i = 0; i < tmpl.m_template_args.positional.size(); i++)
        {
            auto arg_pattern_opt = co_await rpnx::querygraph::query_request< lookup_query >(contextual_type_reference{
                .context = template_context,
                .type = tmpl.m_template_args.positional.at(i).type,
            });

            if (!arg_pattern_opt.has_value())
            {
                matched = false;
                break;
            }

            auto const& arg_pattern = *arg_pattern_opt;
            if (i >= input.parameters.positional.size())
            {
                matched = false;
                break;
            }

            if (!match_template(arg_pattern, input.parameters.positional.at(i)).has_value())
            {
                matched = false;
                break;
            }

            candidate.which.interface.positional.push_back(argif{.type = arg_pattern});
        }

        if (!matched || input.parameters.positional.size() != tmpl.m_template_args.positional.size())
        {
            continue;
        }

        for (auto const& [name, declared_param] : tmpl.m_template_args.named)
        {
            auto named_it = input.parameters.named.find(name);
            if (named_it == input.parameters.named.end())
            {
                matched = false;
                break;
            }

            auto arg_pattern_opt = co_await rpnx::querygraph::query_request< lookup_query >(contextual_type_reference{
                .context = template_context,
                .type = declared_param.type,
            });

            if (!arg_pattern_opt.has_value())
            {
                matched = false;
                break;
            }

            auto const& arg_pattern = *arg_pattern_opt;
            if (!match_template(arg_pattern, named_it->second).has_value())
            {
                matched = false;
                break;
            }

            auto canonical_name = template_parameter_name(declared_param).value_or(name);
            candidate.which.interface.named[canonical_name] = argif{.type = arg_pattern};
        }

        if (!matched || input.parameters.named.size() != tmpl.m_template_args.named.size())
        {
            continue;
        }

        auto priority = tmpl.priority.value_or(0);
        if (!highest_priority.has_value() || priority > *highest_priority)
        {
            highest_priority = priority;
            matches.clear();
            matches.push_back(candidate);
        }
        else if (priority == *highest_priority)
        {
            matches.push_back(candidate);
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
        .temploid = matches.front(),
        .params = input.parameters,
    };
}
