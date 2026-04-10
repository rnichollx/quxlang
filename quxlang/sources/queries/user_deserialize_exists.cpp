// Copyright 2025-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/user_deserialize_exists_spec.hpp>


rpnx::querygraph::coroutine< quxlang::user_deserialize_exists_spec > quxlang::user_deserialize_exists_impl(type_symbol input)
{
    auto deserialize_symbol = submember{.of = input, .name = "DESERIALIZE"};
    auto user_overloads = co_await rpnx::querygraph::request< functum_user_overloads_query >(deserialize_symbol);
    co_return !user_overloads.empty();
}
