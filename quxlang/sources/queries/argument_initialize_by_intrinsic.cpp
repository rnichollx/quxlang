// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/argument_initialize_by_intrinsic_spec.hpp>
#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/manipulators/numeric_literal_utils.hpp"

rpnx::querygraph::coroutine< quxlang::argument_initialize_by_intrinsic_spec > quxlang::argument_initialize_by_intrinsic_impl(argument_init_input input)
{
    auto from = input.from;

    if (typeis< attached_type_reference >(from) || typeis< attached_type_reference >(input.to))
    {
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
