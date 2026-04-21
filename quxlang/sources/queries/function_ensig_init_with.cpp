// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/function_ensig_init_with_spec.hpp>

rpnx::querygraph::coroutine< quxlang::function_ensig_init_with_spec > quxlang::function_ensig_init_with_impl(ensig_initialization input)
{
    co_return co_await rpnx::querygraph::request< ensig_initialize_query >(std::move(input));
}
