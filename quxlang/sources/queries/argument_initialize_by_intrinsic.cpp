// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/argument_initialize_by_intrinsic_spec.hpp>
#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/manipulators/numeric_literal_utils.hpp"

namespace quxlang::detail
{
    /** Returns whether argument adaptation may read a value through a source reference. */
    static auto allows_source_objectization(allowed_adaptations adaptations) -> bool
    {
        switch (adaptations)
        {
        case allowed_adaptations::source_rebinding:
        case allowed_adaptations::class_conversions:
        case allowed_adaptations::destination_rebinding:
            return true;
        case allowed_adaptations::none:
            return false;
        }

        throw compiler_bug("unreachable allowed_adaptations");
    }

    /** Returns the readable pointer value type considered by an intrinsic conversion. */
    static auto intrinsic_pointer_source_type(type_symbol const& source, type_symbol const& destination, allowed_adaptations adaptations) -> type_symbol
    {
        if (is_ref(source) && !is_ref(destination) && !is_write_ref(source) && allows_source_objectization(adaptations))
        {
            type_symbol const objectized = remove_ref(source);
            if (typeis< ptrref_type >(objectized))
            {
                return objectized;
            }
        }

        return source;
    }

    /** Returns whether inheritance conversion preserves the pointer representation category. */
    static auto is_inheritance_pointer_category(pointer_class source, pointer_class destination) -> bool
    {
        return source == destination && (source == pointer_class::ref || source == pointer_class::instance);
    }
}

rpnx::querygraph::coroutine< quxlang::argument_initialize_by_intrinsic_spec > quxlang::argument_initialize_by_intrinsic_impl(argument_init_input input)
{
    auto from = input.from;
    type_symbol const pointer_source_type = detail::intrinsic_pointer_source_type(from, input.to, input.adaptations);

    if (typeis< attached_type_reference >(from) || typeis< attached_type_reference >(input.to))
    {
        co_return std::nullopt;
    }

    if (typeis< ptrref_type >(pointer_source_type) && typeis< ptrref_type >(input.to))
    {
        ptrref_type const& source_pointer = as< ptrref_type >(pointer_source_type);
        ptrref_type const& destination_pointer = as< ptrref_type >(input.to);
        if (detail::is_inheritance_pointer_category(source_pointer.ptr_class, destination_pointer.ptr_class) &&
            qualifier_template_match(destination_pointer.qual, source_pointer.qual).has_value() &&
            source_pointer.target != destination_pointer.target &&
            !is_template(source_pointer.target) &&
            !is_template(destination_pointer.target) &&
            co_await rpnx::querygraph::request< symbol_type_query >(source_pointer.target) == symbol_kind::class_ &&
            co_await rpnx::querygraph::request< symbol_type_query >(destination_pointer.target) == symbol_kind::class_ &&
            co_await rpnx::querygraph::request< class_type_query >(source_pointer.target) == class_kind::struct_ &&
            co_await rpnx::querygraph::request< class_type_query >(destination_pointer.target) == class_kind::struct_)
        {
            struct_conversion_result const conversion = co_await rpnx::querygraph::request< struct_conversion_query >(struct_conversion_input{
                .source_type = source_pointer.target,
                .destination_type = destination_pointer.target,
            });
            if (conversion.status == struct_conversion_status::unique)
            {
                co_return input.to;
            }
        }
    }

    if (typeis< null_type >(from))
    {
        if (typeis< void_type >(input.to))
        {
            co_return input.to;
        }

        if (typeis< ptrref_type >(input.to) && as< ptrref_type >(input.to).ptr_class != pointer_class::ref)
        {
            co_return input.to;
        }

        if (is_template(input.to))
        {
            co_return std::nullopt;
        }

        if (co_await rpnx::querygraph::request< symbol_type_query >(input.to) == symbol_kind::interface_ &&
            co_await rpnx::querygraph::request< interface_defaultable_query >(input.to))
        {
            co_return input.to;
        }

        co_return std::nullopt;
    }

    if (typeis< int_type >(input.to) && typeis< numeric_literal_type >(from))
    {
        auto const& nlt = as< numeric_literal_type >(from);
        if (!literal_fits_int(nlt.value, as< int_type >(input.to)))
        {
            co_return std::nullopt;
        }
        co_return input.to;
    }

    if (typeis< float_type >(input.to) && typeis< numeric_literal_type >(from))
    {
        auto const& nlt = as< numeric_literal_type >(from);
        if (!literal_fits_float(nlt.value, as< float_type >(input.to)))
        {
            co_return std::nullopt;
        }
        co_return input.to;
    }

    if (typeis< readonly_constant >(input.to) && as< readonly_constant >(input.to).kind == constant_kind::numeric && typeis< numeric_literal_type >(from))
    {
        co_return input.to;
    }

    if (typeis< readonly_constant >(input.to) && as< readonly_constant >(input.to).kind == constant_kind::string && typeis< string_literal_type >(from))
    {
        co_return input.to;
    }

    co_return std::nullopt;
}
