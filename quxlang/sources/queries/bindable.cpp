// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/bindable_spec.hpp>
#include "quxlang/manipulators/typeutils.hpp"

rpnx::querygraph::coroutine< quxlang::bindable_spec > quxlang::bindable_impl(implicitly_convertible_to_input input)
{
    auto const& from = input.from;
    auto const& to = input.to;

    assert((!typeis< attached_type_reference >(from) && !typeis< attached_type_reference >(to) && !typeis< attached_type_reference >(remove_ref(from)) && !typeis< attached_type_reference >(remove_ref(to))) && "Bindable resolver does not support symbol-attached types.");

    if (from == to)
    {
        co_return true;
    }

    if (remove_ref(to) != remove_ref(from))
    {
        co_return false;
    }

    if (!is_ref(from) && is_ref(to))
    {
        co_return co_await rpnx::querygraph::request< bindable_by_temporary_materialization_query >(input);
    }

    if (is_ref(from) && is_ref(to))
    {
        co_return co_await rpnx::querygraph::request< bindable_by_reference_requalification_query >(input);
    }

    if (is_ref(from) && !is_ref(to))
    {
        co_return co_await rpnx::querygraph::request< bindable_by_reference_objectization_query >(input);
    }

    co_return false;
}
