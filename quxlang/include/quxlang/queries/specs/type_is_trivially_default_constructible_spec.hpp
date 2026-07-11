// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TYPE_IS_TRIVIALLY_DEFAULT_CONSTRUCTIBLE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TYPE_IS_TRIVIALLY_DEFAULT_CONSTRUCTIBLE_SPEC_HEADER_GUARD

#include <quxlang/queries/struct_layout.hpp>
#include <quxlang/queries/struct_tags.hpp>
#include <quxlang/queries/enum_info.hpp>
#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/type_is_trivially_default_constructible.hpp>
#include <quxlang/queries/user_default_ctor_exists.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct type_is_trivially_default_constructible_spec
    {
        using query = type_is_trivially_default_constructible_query;
        using dependencies = rpnx::typelist< struct_layout_query, struct_tags_query, enum_info_query, class_type_query, symbol_type_query, type_is_trivially_default_constructible_query, user_default_ctor_exists_query >;
    };

    /// Returns true when default construction can be represented as zero-initialized storage.
    rpnx::querygraph::coroutine< type_is_trivially_default_constructible_spec > type_is_trivially_default_constructible_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TYPE_IS_TRIVIALLY_DEFAULT_CONSTRUCTIBLE_SPEC_HEADER_GUARD
