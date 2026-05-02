// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TEMPLEX_INITIALIZE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TEMPLEX_INITIALIZE_SPEC_HEADER_GUARD

#include <quxlang/queries/templex_initialize.hpp>
#include <quxlang/queries/template_instanciation.hpp>
#include <quxlang/queries/templex_select_template.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct templex_initialize_spec
    {
        using query = templex_initialize_query;
        using dependencies = rpnx::typelist< template_instanciation_query, templex_select_template_query >;
    };

    rpnx::querygraph::coroutine< templex_initialize_spec > templex_initialize_impl(initialization_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TEMPLEX_INITIALIZE_SPEC_HEADER_GUARD
