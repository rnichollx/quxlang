// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/keywords.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/specs/type_is_trivially_default_constructible_spec.hpp>

rpnx::querygraph::coroutine< quxlang::type_is_trivially_default_constructible_spec > quxlang::type_is_trivially_default_constructible_impl(type_symbol input)
{
    if (typeis< int_type >(input) || typeis< byte_type >(input) || typeis< bool_type >(input) || typeis< float_type >(input))
    {
        co_return true;
    }

    if (typeis< storage >(input) || typeis< aligned_storage >(input) || typeis< ptrref_type >(input) || typeis< procedure_type >(input))
    {
        co_return true;
    }

    if (typeis< array_type >(input))
    {
        co_return co_await rpnx::querygraph::request< type_is_trivially_default_constructible_query >(as< array_type >(input).element_type);
    }

    if (typeis< attached_type_reference >(input))
    {
        attached_type_reference const& attached = as< attached_type_reference >(input);
        if (typeis< void_type >(attached.carrying_type))
        {
            co_return true;
        }
        co_return co_await rpnx::querygraph::request< type_is_trivially_default_constructible_query >(attached.carrying_type);
    }

    if (atomic_type_argument(input).has_value())
    {
        co_return false;
    }

    symbol_kind const kind = co_await rpnx::querygraph::request< symbol_type_query >(input);
    if (kind != symbol_kind::class_)
    {
        co_return false;
    }

    class_kind const concrete_kind = co_await rpnx::querygraph::request< class_type_query >(input);
    if (concrete_kind == class_kind::union_ || concrete_kind == class_kind::variant)
    {
        co_return false;
    }
    if (concrete_kind == class_kind::flagset)
    {
        co_return true;
    }

    if (concrete_kind == class_kind::enum_)
    {
        enum_info const info = co_await rpnx::querygraph::request< enum_info_query >(input);
        for (enum_value_info const& value : info.values)
        {
            if (value.is_default && value.value == 0)
            {
                co_return true;
            }
        }
        co_return false;
    }

    if (concrete_kind != class_kind::struct_)
    {
        co_return false;
    }

    if (co_await rpnx::querygraph::request< user_default_ctor_exists_query >(input))
    {
        co_return false;
    }

    std::set< std::string > const tags = co_await rpnx::querygraph::request< struct_tags_query >(input);
    if (tags.contains(keywords::no_implicit_default_constructor) || tags.contains(keywords::no_implicit_constructors))
    {
        co_return false;
    }

    struct_layout const layout = co_await rpnx::querygraph::request< struct_layout_query >(input);
    for (struct_field_info const& field : layout.fields)
    {
        if (!(co_await rpnx::querygraph::request< type_is_trivially_default_constructible_query >(field.type)))
        {
            co_return false;
        }
    }

    co_return true;
}
