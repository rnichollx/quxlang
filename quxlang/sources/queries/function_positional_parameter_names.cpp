// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/function_positional_parameter_names_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"

#include <vector>

#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/variant_utils.hpp"

#include <quxlang/macros.hpp>

using namespace quxlang;


rpnx::querygraph::coroutine< quxlang::function_positional_parameter_names_spec > quxlang::function_positional_parameter_names_impl(temploid_reference input)
{
    std::vector< std::optional< std::string > > result;
    auto const& func = co_await rpnx::querygraph::request< function_declaration_query >(input);

    if (!func.has_value())
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

        if (param.name.has_value() && names.contains(*param.name))
        {
            throw std::logic_error("Duplicate parameter name");
        }

        result.push_back(param.name);
    }

    co_return result;
}
