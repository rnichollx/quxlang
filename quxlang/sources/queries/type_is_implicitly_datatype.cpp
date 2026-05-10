// Copyright 2025-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/type_is_implicitly_datatype_spec.hpp>


rpnx::querygraph::coroutine< quxlang::type_is_implicitly_datatype_spec > quxlang::type_is_implicitly_datatype_impl(type_symbol input)
{
    // Check if the type is an int_type, float_type, byte_type, or bool_type
    if (typeis< int_type >(input) || typeis< float_type >(input) || typeis< byte_type >(input) || typeis< bool_type >(input))
    {
        co_return true;
    }

    if (typeis< procedure_type >(input))
    {
        co_return false;
    }

    if (is_atomic_type(input))
    {
        co_return false;
    }

    if (typeis< attached_type_reference >(input))
    {
        attached_type_reference const& attached = as< attached_type_reference >(input);
        if (typeis< void_type >(attached.carrying_type))
        {
            co_return true;
        }
        co_return co_await rpnx::querygraph::request< type_is_implicitly_datatype_query >(attached.carrying_type);
    }

    // Pointers and references are not implicitly datatypes
    if (typeis< ptrref_type >(input) )
    {
        co_return false;
    }

    auto type_kind = co_await rpnx::querygraph::request< symbol_type_query >(input);

    if (type_kind == symbol_kind::enum_ || type_kind == symbol_kind::flagset_)
    {
        co_return true;
    }

    if (type_kind == symbol_kind::class_)
    {


        auto class_fields = co_await rpnx::querygraph::request< class_field_list_query >(input);
        for (const auto& field : class_fields)
        {
            // If any member is not a datatype, the class is not implicitly a datatype
            if (!(co_await rpnx::querygraph::request< type_is_implicitly_datatype_query >(field.type)))
            {
                co_return false;
            }
        }
        co_return true;
    }

    // Default to false for other types
    co_return false;
}
