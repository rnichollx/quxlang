// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_DECLARATION_PRIVACY_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_DECLARATION_PRIVACY_SPEC_HEADER_GUARD

#include <quxlang/queries/active_subdeclaroids.hpp>
#include <quxlang/queries/canonical_lookup.hpp>
#include <quxlang/queries/declaration_privacy.hpp>
#include <quxlang/queries/symboid.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Querygraph handler specification for privacy resolution. */
    struct declaration_privacy_spec
    {
        using query = declaration_privacy_query;
        using dependencies = rpnx::typelist< active_subdeclaroids_query, canonical_lookup_query, symboid_query >;
    };

    /** Resolves source privacy into canonical allowed contexts. */
    rpnx::querygraph::coroutine< declaration_privacy_spec > declaration_privacy_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_DECLARATION_PRIVACY_SPEC_HEADER_GUARD
