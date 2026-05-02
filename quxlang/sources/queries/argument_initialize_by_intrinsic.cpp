// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/argument_initialize_by_intrinsic_spec.hpp>
#include "quxlang/manipulators/typeutils.hpp"

rpnx::querygraph::coroutine< quxlang::argument_initialize_by_intrinsic_spec > quxlang::argument_initialize_by_intrinsic_impl(argument_init_input input)
{
    auto from = input.from;

    if (typeis< attached_type_reference >(from))
    {
        from = as< attached_type_reference >(from).carrying_type;
    }

    if ((typeis< int_type >(input.to) || typeis< float_type >(input.to)) && typeis< numeric_literal_reference >(from))
    {
        co_return input.to;
    }

    if (typeis< readonly_constant >(input.to) && as< readonly_constant >(input.to).kind == constant_kind::string && typeis< string_literal_reference >(from))
    {
        co_return input.to;
    }

    co_return std::nullopt;
}
