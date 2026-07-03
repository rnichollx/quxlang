// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_NUMERIC_STATIC_VALUE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_NUMERIC_STATIC_VALUE_SPEC_HEADER_GUARD

#include <quxlang/queries/constexpr_eval_numeric.hpp>
#include <quxlang/queries/global_is_numeric_static.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/numeric_static_value.hpp>
#include <quxlang/queries/symboid.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct numeric_static_value_spec
    {
        using query = numeric_static_value_query;
        using dependencies = rpnx::typelist< constexpr_eval_numeric_query, global_is_numeric_static_query, lookup_query, symboid_query >;
    };

    rpnx::querygraph::coroutine< numeric_static_value_spec > numeric_static_value_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_NUMERIC_STATIC_VALUE_SPEC_HEADER_GUARD
