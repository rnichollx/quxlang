// Copyright 2025-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/type_should_autogen_deserialize_spec.hpp>


rpnx::querygraph::coroutine< quxlang::type_should_autogen_deserialize_spec > quxlang::type_should_autogen_deserialize_impl(type_symbol input)
{
    if (typeis< array_type >(input))
    {
        type_symbol element = as< array_type >(input).element_type;
        co_return co_await rpnx::querygraph::request< user_deserialize_exists_query >(element)
            || co_await rpnx::querygraph::request< type_should_autogen_deserialize_query >(element);
    }
    // If user-defined deserialize exists, no need to autogen
    if (co_await rpnx::querygraph::request< user_deserialize_exists_query >(input))
    {
        co_return false;
    }

    // If the type is not an implicitly datatype, cannot autogen
    if (!(co_await rpnx::querygraph::request< type_is_implicitly_datatype_query >(input)))
    {
        co_return false;
    }

    // Otherwise, should autogen
    co_return true;
}
