// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_SYMBOID_SUBDECLAROIDS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_SYMBOID_SUBDECLAROIDS_SPEC_HEADER_GUARD

#include <quxlang/queries/symboid_subdeclaroids.hpp>
#include <quxlang/queries/symboid.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct symboid_subdeclaroids_spec
    {
        using query = symboid_subdeclaroids_query;
        using dependencies = rpnx::typelist< symboid_query >;
    };

    rpnx::querygraph::coroutine< symboid_subdeclaroids_spec > symboid_subdeclaroids_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_SYMBOID_SUBDECLAROIDS_SPEC_HEADER_GUARD
