// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CONSTEXPR_ROUTINE_ANTESTATAL_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CONSTEXPR_ROUTINE_ANTESTATAL_SPEC_HEADER_GUARD

#include <quxlang/co_vmir_generator2_deps.hpp>
#include <quxlang/queries/constexpr_routine_antestatal.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct constexpr_routine_antestatal_spec
    {
        using query = constexpr_routine_antestatal_query;
        using dependencies = co_vmir_generator2_query_deps;
    };

    rpnx::querygraph::coroutine< constexpr_routine_antestatal_spec > constexpr_routine_antestatal_impl(constexpr_input2 input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CONSTEXPR_ROUTINE_ANTESTATAL_SPEC_HEADER_GUARD
