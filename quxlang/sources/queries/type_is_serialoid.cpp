// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/keywords.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/type_is_serialoid_spec.hpp>

rpnx::querygraph::coroutine< quxlang::type_is_serialoid_spec > quxlang::type_is_serialoid_impl(type_symbol input)
{
    auto type_kind = co_await rpnx::querygraph::request< symbol_type_query >(input);
    if (type_kind != symbol_kind::class_)
    {
        co_return false;
    }

    auto tags = co_await rpnx::querygraph::request< class_tags_query >(input);
    if (tags.contains(keywords::nonstatic))
    {
        co_return false;
    }
    if (tags.contains(keywords::antestatal))
    {
        co_return false;
    }

    auto has_user_serialize = co_await rpnx::querygraph::request< user_serialize_exists_query >(input);
    auto has_user_deserialize = co_await rpnx::querygraph::request< user_deserialize_exists_query >(input);

    if (tags.contains(keywords::serialoid))
    {
        auto can_serialize = has_user_serialize || co_await rpnx::querygraph::request< type_should_autogen_serialize_query >(input);
        auto can_deserialize = has_user_deserialize || co_await rpnx::querygraph::request< type_should_autogen_deserialize_query >(input);
        if (!can_serialize || !can_deserialize)
        {
            throw semantic_compilation_error("SERIALOID type does not have serialization and deserialization support: " + quxlang::to_string(input));
        }
        co_return true;
    }

    co_return has_user_serialize && has_user_deserialize;
}
