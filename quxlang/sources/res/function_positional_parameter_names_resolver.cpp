// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/res/function_positional_parameter_names_resolver.hpp>

#include <quxlang/compiler.hpp>
#include <quxlang/macros.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(function_positional_parameter_names)
{
    std::vector< std::string > result;
    auto const& func = co_await QUX_CO_DEP(function_declaration, (input_val));

    if (! func.has_value())
    {
        throw std::logic_error("Function not found");
    }

    std::set< std::string > names;

    for (auto const& param : func->header.call_parameters)
    {
        if (param.api_name.has_value())
        {
            // non-positional parameter
            continue;
        }

        if (names.contains(param.name))
        {
            throw std::logic_error("Duplicate parameter name");
        }

        result.push_back(param.name);
    }

    QUX_CO_ANSWER(result);
}
