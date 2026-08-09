// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_INCLUDE_IF_IS_ACTIVE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_INCLUDE_IF_IS_ACTIVE_SPEC_HEADER_GUARD

#include <quxlang/queries/constexpr_bool.hpp>
#include <quxlang/queries/include_if_is_active.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Querygraph handler specification for optional INCLUDE_IF evaluation. */
    struct include_if_is_active_spec
    {
        using query = include_if_is_active_query;
        using dependencies = rpnx::typelist< constexpr_bool_query >;
    };

    /** Evaluates an optional INCLUDE_IF condition in its containing scope. */
    rpnx::querygraph::coroutine< include_if_is_active_spec > include_if_is_active_impl(include_if_is_active_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_INCLUDE_IF_IS_ACTIVE_SPEC_HEADER_GUARD
