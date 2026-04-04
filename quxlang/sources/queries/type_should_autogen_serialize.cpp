// Copyright 2025-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/type_should_autogen_serialize_spec.hpp>


rpnx::querygraph::coroutine< quxlang::type_should_autogen_serialize_spec > quxlang::type_should_autogen_serialize_impl(type_symbol input)
{
    // If user-defined serialize exists, no need to autogen
    if (co_await rpnx::querygraph::query_request< user_serialize_exists_query >(input))
    {
        co_return false;
    }

    // If the type is not an implicitly datatype, cannot autogen
    if (!(co_await rpnx::querygraph::query_request< type_is_implicitly_datatype_query >(input)))
    {
        co_return false;
    }

    // Otherwise, should autogen
    co_return true;
}
