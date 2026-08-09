// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_ACTIVE_SYMBOID_SUBDECLAROIDS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_ACTIVE_SYMBOID_SUBDECLAROIDS_SPEC_HEADER_GUARD

#include <quxlang/queries/active_symboid_subdeclaroids.hpp>
#include <quxlang/queries/include_if_is_active.hpp>
#include <quxlang/queries/symboid_subdeclaroids.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Querygraph handler specification for active declarations directly contained by one symbol. */
    struct active_symboid_subdeclaroids_spec
    {
        using query = active_symboid_subdeclaroids_query;
        using dependencies = rpnx::typelist< include_if_is_active_query, symboid_subdeclaroids_query >;
    };

    /** Resolves INCLUDE_IF for every declaration directly contained by one canonical symbol. */
    rpnx::querygraph::coroutine< active_symboid_subdeclaroids_spec > active_symboid_subdeclaroids_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_ACTIVE_SYMBOID_SUBDECLAROIDS_SPEC_HEADER_GUARD
