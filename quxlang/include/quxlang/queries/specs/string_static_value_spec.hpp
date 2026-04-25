// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_STRING_STATIC_VALUE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_STRING_STATIC_VALUE_SPEC_HEADER_GUARD

#include <quxlang/queries/constexpr_eval_string.hpp>
#include <quxlang/queries/global_is_string_static.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/string_static_value.hpp>
#include <quxlang/queries/symboid.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using string_static_value_spec = rpnx::querygraph::query_handler_spec<
        string_static_value_query,
        rpnx::typelist< constexpr_eval_string_query, global_is_string_static_query, lookup_query, symboid_query > >;

    /// Computes the constexpr byte contents for a STATIC STRING_CONSTANT global initializer.
    rpnx::querygraph::coroutine< string_static_value_spec > string_static_value_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_STRING_STATIC_VALUE_SPEC_HEADER_GUARD
