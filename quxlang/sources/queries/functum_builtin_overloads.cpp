// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/functum_builtin_overloads_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/debug.hpp"
#include <vector>

#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/variant_utils.hpp"

#include <quxlang/macros.hpp>

using namespace quxlang;


rpnx::querygraph::coroutine< quxlang::functum_builtin_overloads_spec > quxlang::functum_builtin_overloads_impl(type_symbol input)
{
    auto const& builtin_infos = co_await rpnx::querygraph::query_request< functum_builtins_query >(input);
    std::set< temploid_ensig > results;
    for (auto const& info : builtin_infos)
    {
        results.insert(info.overload);
    }
    co_return results;
}
