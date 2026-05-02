// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CONSTEXPR_ROUTINE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CONSTEXPR_ROUTINE_SPEC_HEADER_GUARD

#include <quxlang/co_vmir_generator2_deps.hpp>
#include <quxlang/queries/constexpr_routine.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct constexpr_routine_spec
    {
        using query = constexpr_routine_query;
        using dependencies = co_vmir_generator2_query_deps;
    };

    rpnx::querygraph::coroutine< constexpr_routine_spec > constexpr_routine_impl(constexpr_input2 input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CONSTEXPR_ROUTINE_SPEC_HEADER_GUARD
