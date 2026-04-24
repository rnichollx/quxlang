// Copyright 2025-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/user_deserialize_exists_spec.hpp>


rpnx::querygraph::coroutine< quxlang::user_deserialize_exists_spec > quxlang::user_deserialize_exists_impl(type_symbol input)
{
    auto deserialize_symbol = submember{.of = input, .name = "DESERIALIZE"};
    auto user_overloads = co_await rpnx::querygraph::request< functum_user_overloads_query >(deserialize_symbol);
    if (!user_overloads.empty())
    {
        co_return true;
    }

    auto constructor_symbol = submember{.of = input, .name = "CONSTRUCTOR"};
    auto constructor_overloads = co_await rpnx::querygraph::request< functum_user_overloads_query >(constructor_symbol);
    for (auto const& overload : constructor_overloads)
    {
        if (overload.interface.named.contains("DESERIALIZE_INPUT_ITERATOR"))
        {
            co_return true;
        }
    }

    co_return false;
}
