// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/include_if_is_active_spec.hpp>

#include <utility>

rpnx::querygraph::coroutine< quxlang::include_if_is_active_spec > quxlang::include_if_is_active_impl(include_if_is_active_input input)
{
    if (!input.condition.has_value())
    {
        co_return true;
    }

    constexpr_input condition{
        .context = std::move(input.context),
        .expr = std::move(*input.condition),
    };
    co_return co_await rpnx::querygraph::request< constexpr_bool_query >(std::move(condition));
}
