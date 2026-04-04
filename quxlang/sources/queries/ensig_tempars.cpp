// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/ensig_tempars_spec.hpp>

rpnx::querygraph::coroutine< quxlang::ensig_tempars_spec > quxlang::ensig_tempars_impl(temploid_ensig input)
{
    tempar_name_set result;

    for (auto const& param : input.interface.positional)
    {
        auto param_tempars = co_await rpnx::querygraph::query_request< symbol_tempars_query >(param.type);
        result.insert(param_tempars.begin(), param_tempars.end());
    }

    for (auto const& [name, param] : input.interface.named)
    {
        (void)name;
        auto param_tempars = co_await rpnx::querygraph::query_request< symbol_tempars_query >(param.type);
        result.insert(param_tempars.begin(), param_tempars.end());
    }

    co_return result;
}
