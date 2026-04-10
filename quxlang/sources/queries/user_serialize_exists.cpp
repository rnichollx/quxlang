// Copyright 2025-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/user_serialize_exists_spec.hpp>


rpnx::querygraph::coroutine< quxlang::user_serialize_exists_spec > quxlang::user_serialize_exists_impl(type_symbol input)
{
    auto serialize_symbol = submember{.of = input, .name = "SERIALIZE"};
    auto user_overloads = co_await rpnx::querygraph::request< functum_user_overloads_query >(serialize_symbol);
    co_return !user_overloads.empty();
}
