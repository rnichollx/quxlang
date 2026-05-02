// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTUM_INITIALIZE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTUM_INITIALIZE_SPEC_HEADER_GUARD

#include <quxlang/queries/functum_initialize.hpp>
#include <quxlang/queries/function_instanciation.hpp>
#include <quxlang/queries/functum_select_function.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct functum_initialize_spec
    {
        using query = functum_initialize_query;
        using dependencies = rpnx::typelist< function_instanciation_query, functum_select_function_query >;
    };

    rpnx::querygraph::coroutine< functum_initialize_spec > functum_initialize_impl(initialization_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTUM_INITIALIZE_SPEC_HEADER_GUARD
