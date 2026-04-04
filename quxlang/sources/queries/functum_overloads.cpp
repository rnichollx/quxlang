// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/functum_overloads_spec.hpp>

#include "quxlang/operators.hpp"


rpnx::querygraph::coroutine< quxlang::functum_overloads_spec > quxlang::functum_overloads_impl(type_symbol input)
{

    auto const & builtins = co_await rpnx::querygraph::query_request< functum_builtin_overloads_query >(input);
    auto const & user = co_await rpnx::querygraph::query_request< functum_user_overloads_query >(input);

    std::set<temploid_ensig> results;

    for (auto const & ol : builtins)
    {
        results.insert(ol);
    }

    for (auto const & ol : user)
    {
        results.insert(ol);
    }

    co_return results;
}
