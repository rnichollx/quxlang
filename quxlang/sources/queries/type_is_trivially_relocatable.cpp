// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/exception.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/specs/type_is_trivially_relocatable_spec.hpp>

rpnx::querygraph::coroutine< quxlang::type_is_trivially_relocatable_spec > quxlang::type_is_trivially_relocatable_impl(type_symbol input)
{
    if (typeis< size_type >(input) || type_is_contextual(input))
    {
        throw compiler_bug("type_is_trivially_relocatable received a non-canonical type: " + to_string(input));
    }

    if (typeis< void_type >(input) || typeis< int_type >(input) || typeis< byte_type >(input) || typeis< bool_type >(input) || typeis< float_type >(input))
    {
        co_return true;
    }

    if (typeis< storage >(input) || typeis< aligned_storage >(input) || typeis< ptrref_type >(input) || typeis< procedure_type >(input) ||
        typeis< initguard_lock_type >(input) || typeis< readonly_constant >(input) || typeis< array_initializer_type >(input) || typeis< address_type >(input))
    {
        co_return true;
    }

    if (typeis< array_type >(input))
    {
        co_return co_await rpnx::querygraph::request< type_is_trivially_relocatable_query >(as< array_type >(input).element_type);
    }

    if (typeis< attached_type_reference >(input))
    {
        attached_type_reference const& attached = as< attached_type_reference >(input);
        if (typeis< void_type >(attached.carrying_type))
        {
            co_return true;
        }
        co_return co_await rpnx::querygraph::request< type_is_trivially_relocatable_query >(attached.carrying_type);
    }

    if (atomic_type_argument(input).has_value())
    {
        co_return false;
    }

    symbol_kind const kind = co_await rpnx::querygraph::request< symbol_type_query >(input);
    if (kind == symbol_kind::interface_)
    {
        co_return true;
    }
    if (kind == symbol_kind::class_)
    {
        class_kind const concrete_kind = co_await rpnx::querygraph::request< class_type_query >(input);
        if (concrete_kind == class_kind::union_ || concrete_kind == class_kind::variant)
        {
            co_return false;
        }
        co_return concrete_kind == class_kind::enum_ || concrete_kind == class_kind::flagset;
    }

    co_return false;
}
