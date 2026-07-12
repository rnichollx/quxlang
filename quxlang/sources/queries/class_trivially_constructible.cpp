// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/class_trivially_constructible_spec.hpp>
#include <quxlang/data/basic_types.hpp>
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/unimplemented.hpp"


rpnx::querygraph::coroutine< quxlang::class_trivially_constructible_spec > quxlang::class_trivially_constructible_impl(type_symbol input)
{
    class_kind const concrete_kind = co_await rpnx::querygraph::request< class_type_query >(input);
    if (concrete_kind == class_kind::union_ || concrete_kind == class_kind::variant)
    {
        co_return false;
    }
    co_return (co_await rpnx::querygraph::request< class_default_ctor_query >(input)).has_value() == false;
}
