// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_DECLARATION_IS_ACCESSIBLE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_DECLARATION_IS_ACCESSIBLE_SPEC_HEADER_GUARD

#include <quxlang/queries/declaration_is_accessible.hpp>
#include <quxlang/queries/declaration_privacy.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Querygraph handler specification for declaration access checks. */
    struct declaration_is_accessible_spec
    {
        using query = declaration_is_accessible_query;
        using dependencies = rpnx::typelist< declaration_privacy_query >;
    };

    /** Evaluates source access against canonical privacy scopes. */
    rpnx::querygraph::coroutine< declaration_is_accessible_spec > declaration_is_accessible_impl(declaration_access_request input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_DECLARATION_IS_ACCESSIBLE_SPEC_HEADER_GUARD
