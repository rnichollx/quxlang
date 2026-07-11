// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TYPE_IS_SERIALOID_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TYPE_IS_SERIALOID_SPEC_HEADER_GUARD

#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/struct_tags.hpp>
#include <quxlang/queries/type_is_implicitly_datatype.hpp>
#include <quxlang/queries/type_is_serialoid.hpp>
#include <quxlang/queries/type_should_autogen_deserialize.hpp>
#include <quxlang/queries/type_should_autogen_serialize.hpp>
#include <quxlang/queries/user_deserialize_exists.hpp>
#include <quxlang/queries/user_serialize_exists.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct type_is_serialoid_spec
    {
        using query = type_is_serialoid_query;
        using dependencies = rpnx::typelist< class_type_query, struct_tags_query, type_is_implicitly_datatype_query, type_should_autogen_deserialize_query, type_should_autogen_serialize_query, user_deserialize_exists_query, user_serialize_exists_query >;
    };

    rpnx::querygraph::coroutine< type_is_serialoid_spec > type_is_serialoid_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TYPE_IS_SERIALOID_SPEC_HEADER_GUARD
