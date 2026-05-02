// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TYPE_IS_STRINGLIKE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TYPE_IS_STRINGLIKE_SPEC_HEADER_GUARD

#include <quxlang/queries/class_tags.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/type_is_stringlike.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct type_is_stringlike_spec
    {
        using query = type_is_stringlike_query;
        using dependencies = rpnx::typelist< class_tags_query, symbol_type_query >;
    };

    /// Returns true when the input symbol is a class declaration carrying the STRINGLIKE tag.
    rpnx::querygraph::coroutine< type_is_stringlike_spec > type_is_stringlike_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TYPE_IS_STRINGLIKE_SPEC_HEADER_GUARD
