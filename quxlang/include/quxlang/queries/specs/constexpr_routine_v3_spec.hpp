// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CONSTEXPR_ROUTINE_V3_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CONSTEXPR_ROUTINE_V3_SPEC_HEADER_GUARD

#include <quxlang/co_vmir_generator2_deps.hpp>
#include <quxlang/queries/constexpr_routine_v3.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using constexpr_routine_v3_spec = rpnx::querygraph::query_handler_spec< constexpr_routine_v3_query, co_vmir_generator2_query_deps >;

    /// Generates a constexpr v3 VMIR routine and primary-result type metadata.
    rpnx::querygraph::coroutine< constexpr_routine_v3_spec > constexpr_routine_v3_impl(constexpr_input_v3 input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CONSTEXPR_ROUTINE_V3_SPEC_HEADER_GUARD
