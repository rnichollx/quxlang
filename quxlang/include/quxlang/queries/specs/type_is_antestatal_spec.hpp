// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TYPE_IS_ANTESTATAL_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TYPE_IS_ANTESTATAL_SPEC_HEADER_GUARD

#include <quxlang/queries/type_is_antestatal.hpp>
#include <quxlang/queries/struct_field_list.hpp>
#include <quxlang/queries/struct_tags.hpp>
#include <quxlang/queries/class_trivially_destructible.hpp>
#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/user_deserialize_exists.hpp>
#include <quxlang/queries/user_serialize_exists.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct type_is_antestatal_spec
    {
        using query = type_is_antestatal_query;
        using dependencies = rpnx::typelist< struct_field_list_query, struct_tags_query, class_trivially_destructible_query, class_type_query, symbol_type_query, type_is_antestatal_query, user_deserialize_exists_query, user_serialize_exists_query >;
    };

    rpnx::querygraph::coroutine< type_is_antestatal_spec > type_is_antestatal_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TYPE_IS_ANTESTATAL_SPEC_HEADER_GUARD
