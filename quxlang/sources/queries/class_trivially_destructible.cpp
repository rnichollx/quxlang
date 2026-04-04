// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/class_trivially_destructible_spec.hpp>
#include "quxlang/data/expression.hpp"
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/value.hpp"


rpnx::querygraph::coroutine< quxlang::class_trivially_destructible_spec > quxlang::class_trivially_destructible_impl(type_symbol input)
{
    co_return (co_await rpnx::querygraph::query_request< class_default_dtor_query >(input)).has_value() == false;
}
