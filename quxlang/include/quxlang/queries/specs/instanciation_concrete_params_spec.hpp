// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_INSTANCIATION_CONCRETE_PARAMS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_INSTANCIATION_CONCRETE_PARAMS_SPEC_HEADER_GUARD

#include <quxlang/queries/instanciation_concrete_params.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/constexpr_u64.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct instanciation_concrete_params_spec
    {
        using query = instanciation_concrete_params_query;
        using dependencies = rpnx::typelist< lookup_query, constexpr_u64_query >;
    };

    rpnx::querygraph::coroutine< instanciation_concrete_params_spec > instanciation_concrete_params_impl(instanciation_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_INSTANCIATION_CONCRETE_PARAMS_SPEC_HEADER_GUARD
