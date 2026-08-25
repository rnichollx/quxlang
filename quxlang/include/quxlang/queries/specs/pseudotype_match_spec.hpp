// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_PSEUDOTYPE_MATCH_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_PSEUDOTYPE_MATCH_SPEC_HEADER_GUARD

#include <quxlang/queries/pseudotype_match.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Supplies exact structural pseudotype matching. */
    struct pseudotype_match_spec
    {
        using query = pseudotype_match_query;
        using dependencies = rpnx::typelist<>;
    };

    /** Matches a pseudotype without resolving or converting either input. */
    rpnx::querygraph::coroutine< pseudotype_match_spec > pseudotype_match_impl(pseudotype_match_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_PSEUDOTYPE_MATCH_SPEC_HEADER_GUARD
