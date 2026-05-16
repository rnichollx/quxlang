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
    auto const& user_overloads = co_await rpnx::querygraph::request< functum_map_user_formal_ensigs_query >(input.templexoid);
    auto primitive_overloads = co_await rpnx::querygraph::request< functum_builtins_query >(input.templexoid);
    auto const total_count = user_overloads.size() + primitive_overloads.size();

    std::optional< std::uint64_t > builtin_index;
    if (!input.overload_id.has_value())
    {
        if (user_overloads.empty() && total_count == 1)
        {
            builtin_index = 0;
        }
    }
    else if (*input.overload_id >= user_overloads.size() && *input.overload_id < total_count)
    {
        builtin_index = *input.overload_id - static_cast< std::uint64_t >(user_overloads.size());
    }

    if (!builtin_index.has_value())
    {
        co_return std::nullopt;
    }

    std::uint64_t current_index = 0;
    for (auto const& info : primitive_overloads)
    {
        if (current_index == *builtin_index)
        {
            co_return info;
        }
        current_index++;
    }

    co_return std::nullopt;
}
