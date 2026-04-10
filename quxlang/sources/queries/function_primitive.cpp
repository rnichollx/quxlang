// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/function_primitive_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"

#include <vector>

#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/variant_utils.hpp"

#include <quxlang/macros.hpp>

using namespace quxlang;


rpnx::querygraph::coroutine< quxlang::function_primitive_spec > quxlang::function_primitive_impl(temploid_reference input)
{
    auto primitive_overloads = co_await rpnx::querygraph::request< functum_builtins_query >(input.templexoid);

    for (auto const& info : primitive_overloads)
    {
        if (info.overload == input.which)
        {
            co_return info;
        }
    }

    co_return std::nullopt;
}
