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
    auto const& user_overloads = co_await rpnx::querygraph::request< functum_map_user_formal_ensigs_query >(input.templexoid);
    auto builtin_overloads = co_await rpnx::querygraph::request< functum_builtin_overloads_query >(input.templexoid);
    auto const total_count = user_overloads.size() + builtin_overloads.size();

    if (!input.overload_id.has_value())
    {
        co_return user_overloads.empty() && total_count == 1;
    }

    if (*input.overload_id < user_overloads.size())
    {
        co_return false;
    }

    co_return *input.overload_id < total_count;
}
