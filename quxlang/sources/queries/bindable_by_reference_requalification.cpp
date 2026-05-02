// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/bindable_by_reference_requalification_spec.hpp>
#include "quxlang/manipulators/typeutils.hpp"

rpnx::querygraph::coroutine< quxlang::bindable_by_reference_requalification_spec > quxlang::bindable_by_reference_requalification_impl(implicitly_convertible_to_input input)
{
    auto const& from = input.from;
    auto const& to = input.to;

    if (remove_ref(to) != remove_ref(from) || !is_ref(to) || !is_ref(from))
    {
        co_return false;
    }

    auto const& from_ref = as< ptrref_type >(from);
    auto const& to_ref = as< ptrref_type >(to);

    co_return qualifier_template_match(to_ref.qual, from_ref.qual).has_value();
}
