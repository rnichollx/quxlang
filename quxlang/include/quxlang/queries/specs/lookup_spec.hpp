// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_LOOKUP_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_LOOKUP_SPEC_HEADER_GUARD

#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/canonical_lookup.hpp>
#include <quxlang/queries/declaration_is_accessible.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct lookup_spec
    {
        using query = lookup_query;
        using dependencies = rpnx::typelist< canonical_lookup_query, declaration_is_accessible_query >;
    };

    rpnx::querygraph::coroutine< lookup_spec > lookup_impl(contextual_type_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_LOOKUP_SPEC_HEADER_GUARD
