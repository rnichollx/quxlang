// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/exists_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"

#include "quxlang/manipulators/typeutils.hpp"


rpnx::querygraph::coroutine< quxlang::exists_spec > quxlang::exists_impl(type_symbol input)
{
    auto typ = co_await rpnx::querygraph::query_request< symbol_type_query >(input);
    co_return typ != symbol_kind::noexist;
}
