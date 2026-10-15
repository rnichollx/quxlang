// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_SUBDECLARATIONS_OF_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_SUBDECLARATIONS_OF_SPEC_HEADER_GUARD

#include <quxlang/queries/subdeclarations_of.hpp>
#include <quxlang/queries/symboid.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Querygraph specification for unaltered child declarations. */
    struct subdeclarations_of_spec
    {
        using query = subdeclarations_of_query;
        using dependencies = rpnx::typelist< symboid_query >;
    };

    /** Returns stored subdeclarations without filtering, resolution, or synthesis. */
    rpnx::querygraph::coroutine< subdeclarations_of_spec > subdeclarations_of_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_SUBDECLARATIONS_OF_SPEC_HEADER_GUARD
