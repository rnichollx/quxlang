// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_ACTIVE_SUBDECLAROIDS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_ACTIVE_SUBDECLAROIDS_SPEC_HEADER_GUARD

#include <quxlang/queries/active_subdeclaroids.hpp>
#include <quxlang/queries/include_if_is_active.hpp>
#include <quxlang/queries/symboid_subdeclaroids.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Querygraph handler specification for the active declarations defining one symbol. */
    struct active_subdeclaroids_spec
    {
        using query = active_subdeclaroids_query;
        using dependencies = rpnx::typelist< include_if_is_active_query, symboid_subdeclaroids_query >;
    };

    /** Resolves INCLUDE_IF for the declarations defining one canonical child symbol. */
    rpnx::querygraph::coroutine< active_subdeclaroids_spec > active_subdeclaroids_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_ACTIVE_SUBDECLAROIDS_SPEC_HEADER_GUARD
