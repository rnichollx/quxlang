// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TYPE_SHOULD_AUTOGEN_DESERIALIZE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TYPE_SHOULD_AUTOGEN_DESERIALIZE_SPEC_HEADER_GUARD

#include <quxlang/queries/type_should_autogen_deserialize.hpp>
#include <quxlang/queries/type_is_implicitly_datatype.hpp>
#include <quxlang/queries/user_deserialize_exists.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct type_should_autogen_deserialize_spec
    {
        using query = type_should_autogen_deserialize_query;
        using dependencies = rpnx::typelist< type_is_implicitly_datatype_query, user_deserialize_exists_query >;
    };

    rpnx::querygraph::coroutine< type_should_autogen_deserialize_spec > type_should_autogen_deserialize_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TYPE_SHOULD_AUTOGEN_DESERIALIZE_SPEC_HEADER_GUARD
