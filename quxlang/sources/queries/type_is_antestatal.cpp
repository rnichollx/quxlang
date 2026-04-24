// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/keywords.hpp>
#include <quxlang/queries/specs/type_is_antestatal_spec.hpp>

#include <stdexcept>


rpnx::querygraph::coroutine< quxlang::type_is_antestatal_spec > quxlang::type_is_antestatal_impl(type_symbol input)
{
    if (typeis< nvalue_slot >(input))
    {
        co_return co_await rpnx::querygraph::request< type_is_antestatal_query >(as< nvalue_slot >(input).target);
    }

    if (typeis< dvalue_slot >(input))
    {
        co_return co_await rpnx::querygraph::request< type_is_antestatal_query >(as< dvalue_slot >(input).target);
    }

    if (typeis< int_type >(input) || typeis< bool_type >(input) || typeis< byte_type >(input))
    {
        co_return true;
    }

    if (typeis< ptrref_type >(input))
    {
        co_return true;
    }

    if (typeis< array_type >(input))
    {
        co_return co_await rpnx::querygraph::request< type_is_antestatal_query >(as< array_type >(input).element_type);
    }

    if (typeis< procedure_type >(input) || typeis< storage >(input) || typeis< aligned_storage >(input) || typeis< readonly_constant >(input))
    {
        co_return false;
    }

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
    if (tags.contains(keywords::serialoid))
    {
        co_return false;
    }
    bool const explicitly_antestatal = tags.contains(keywords::antestatal);
    if (!explicitly_antestatal && co_await rpnx::querygraph::request< user_serialize_exists_query >(input) && co_await rpnx::querygraph::request< user_deserialize_exists_query >(input))
    {
        co_return false;
    }

    if (!(co_await rpnx::querygraph::request< class_trivially_destructible_query >(input)))
    {
        if (explicitly_antestatal)
        {
            throw std::logic_error("ANTESTATAL type is not trivially destructible: " + quxlang::to_string(input));
        }
        co_return false;
    }

    auto class_fields = co_await rpnx::querygraph::request< class_field_list_query >(input);
    for (auto const& field : class_fields)
    {
        if (!(co_await rpnx::querygraph::request< type_is_antestatal_query >(field.type)))
        {
            if (explicitly_antestatal)
            {
                throw std::logic_error("ANTESTATAL type has a non-antestatal field: " + quxlang::to_string(input));
            }
            co_return false;
        }
    }

    co_return true;
}
