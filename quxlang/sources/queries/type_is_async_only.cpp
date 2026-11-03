// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/type_is_async_only_spec.hpp>

rpnx::querygraph::coroutine< quxlang::type_is_async_only_spec > quxlang::type_is_async_only_impl(type_symbol input)
{
    type_destruction_capabilities const& capabilities = co_await rpnx::querygraph::request< type_destruction_capabilities_query >(input);
    co_return capabilities.is_async_only;
}
