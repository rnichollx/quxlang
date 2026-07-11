// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/keywords.hpp>
#include <quxlang/queries/specs/type_is_stringlike_spec.hpp>

/// Determines whether the input names a struct class carrying the STRINGLIKE keyword tag.
rpnx::querygraph::coroutine< quxlang::type_is_stringlike_spec > quxlang::type_is_stringlike_impl(type_symbol input)
{
    class_kind const type_kind = co_await rpnx::querygraph::request< class_type_query >(input);
    if (type_kind != class_kind::struct_)
    {
        co_return false;
    }

    struct_tags_result_type const tags = co_await rpnx::querygraph::request< struct_tags_query >(input);
    co_return tags.contains(keywords::stringlike);
}
