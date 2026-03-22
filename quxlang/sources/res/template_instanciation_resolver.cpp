// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/compiler.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/res/template_instanciation.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(template_instanciation)
{
    type_symbol initializee = input_val.initializee;

    if (typeis< temploid_reference >(initializee))
    {
        auto const& temploid = as< temploid_reference >(initializee);

        auto temploid_kind = co_await QUX_CO_DEP(symbol_type, (temploid));
        if (temploid_kind != symbol_kind::template_)
        {
            throw std::logic_error("template_instanciation called on a non-template selection");
        }

        co_return instanciation_reference{
            .temploid = temploid,
            .params = input_val.parameters,
        };
    }

    auto initializee_kind = co_await QUX_CO_DEP(symbol_type, (initializee));
    if (initializee_kind != symbol_kind::templex)
    {
        throw std::logic_error("template_instanciation called on a non-templex");
    }

    auto const& sym = co_await QUX_CO_DEP(symboid, (initializee));
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
        if (!input_val.parameters.named.empty())
        {
            continue;
        }

        if (tmpl.m_template_args.size() != input_val.parameters.positional.size())
        {
            continue;
        }

        temploid_reference candidate;
        candidate.templexoid = initializee;
        candidate.which.priority = tmpl.priority;

        bool matched = true;
        for (std::size_t i = 0; i < tmpl.m_template_args.size(); i++)
        {
            auto arg_pattern_opt = co_await QUX_CO_DEP(lookup, (contextual_type_reference{
                .context = template_context,
                .type = tmpl.m_template_args.at(i),
            }));

            if (!arg_pattern_opt.has_value())
            {
                matched = false;
                break;
            }

            auto const& arg_pattern = *arg_pattern_opt;
            if (!match_template(arg_pattern, input_val.parameters.positional.at(i)).has_value())
            {
                matched = false;
                break;
            }

            candidate.which.interface.positional.push_back(argif{.type = arg_pattern});
        }

        if (!matched)
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
        .params = input_val.parameters,
    };
}
