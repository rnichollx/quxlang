// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/function_builtin_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"
#include <vector>

#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/variant_utils.hpp"

#include <quxlang/macros.hpp>

using namespace quxlang;


rpnx::querygraph::coroutine< quxlang::function_builtin_spec > quxlang::function_builtin_impl(temploid_reference input)
{
    auto builtin_overloads = co_await rpnx::querygraph::request< functum_builtin_overloads_query >(input.templexoid);

    for (auto const& info : builtin_overloads)
    {
        if (info == input.which)
        {
            co_return true;
        }
    }

    co_return false;
}
