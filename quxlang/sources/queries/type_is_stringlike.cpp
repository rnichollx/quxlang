// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/keywords.hpp>
#include <quxlang/queries/specs/type_is_stringlike_spec.hpp>

/// Determines whether the input names a class and that class has the STRINGLIKE keyword tag.
rpnx::querygraph::coroutine< quxlang::type_is_stringlike_spec > quxlang::type_is_stringlike_impl(type_symbol input)
{
    auto type_kind = co_await rpnx::querygraph::request< symbol_type_query >(input);
    if (type_kind != symbol_kind::class_)
    {
        co_return false;
    }

    auto tags = co_await rpnx::querygraph::request< class_tags_query >(input);
    co_return tags.contains(keywords::stringlike);
}
