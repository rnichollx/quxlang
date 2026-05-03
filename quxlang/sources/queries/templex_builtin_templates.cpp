// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/templex_builtin_templates_spec.hpp>

#include <optional>
#include <string>

rpnx::querygraph::coroutine< quxlang::templex_builtin_templates_spec > quxlang::templex_builtin_templates_impl(type_symbol input)
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

    auto temploid_ensig_from_declared_parameters = [&](declared_parameters const& params, std::optional< std::int64_t > priority) -> temploid_ensig
    {
        temploid_ensig ensig;
        ensig.priority = priority;

        for (auto const& param : params.positional)
        {
            ensig.interface.positional.push_back(argif{
                .type = param.type,
                .requires_static_value = param.kind == template_parameter_kind::value,
            });
        }

        for (auto const& [name, param] : params.named)
        {
            auto canonical_name = template_parameter_name(param).value_or(name);
            ensig.interface.named[canonical_name] = argif{
                .type = param.type,
                .requires_static_value = param.kind == template_parameter_kind::value,
            };
        }

        return ensig;
    };

    auto const& builtin_infos = co_await rpnx::querygraph::request< templex_builtins_query >(input);
    std::set< temploid_ensig > results;
    for (auto const& info : builtin_infos)
    {
        results.insert(temploid_ensig_from_declared_parameters(info.template_args, info.priority));
    }
    co_return results;
}
