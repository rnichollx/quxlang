// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TYPE_SHOULD_AUTOGEN_SERIALIZE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TYPE_SHOULD_AUTOGEN_SERIALIZE_SPEC_HEADER_GUARD

#include <quxlang/queries/type_should_autogen_serialize.hpp>
#include <quxlang/queries/type_is_implicitly_datatype.hpp>
#include <quxlang/queries/user_serialize_exists.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using type_should_autogen_serialize_spec = rpnx::querygraph::query_handler_spec< type_should_autogen_serialize_query, rpnx::typelist< type_is_implicitly_datatype_query, user_serialize_exists_query > >;

    rpnx::querygraph::coroutine< type_should_autogen_serialize_spec > type_should_autogen_serialize_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TYPE_SHOULD_AUTOGEN_SERIALIZE_SPEC_HEADER_GUARD
