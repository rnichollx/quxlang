// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_ACTIVE_SUBDECLARATIONS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_ACTIVE_SUBDECLARATIONS_SPEC_HEADER_GUARD

#include <quxlang/queries/active_subdeclarations.hpp>
#include <quxlang/queries/include_if_is_active.hpp>
#include <quxlang/queries/subdeclarations_of.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Querygraph handler specification for the active declarations defining one symbol. */
    struct active_subdeclarations_spec
    {
        using query = active_subdeclarations_query;
        using dependencies = rpnx::typelist< include_if_is_active_query, subdeclarations_of_query >;
    };

    /** Resolves INCLUDE_IF for the declarations defining one canonical child symbol. */
    rpnx::querygraph::coroutine< active_subdeclarations_spec > active_subdeclarations_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_ACTIVE_SUBDECLARATIONS_SPEC_HEADER_GUARD
