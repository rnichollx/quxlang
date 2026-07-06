// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_SUBTAG_BINDING_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_SUBTAG_BINDING_SPEC_HEADER_GUARD

#include <quxlang/queries/subtag_binding.hpp>
#include <quxlang/queries/instanciation_subtag_bindings.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /// Querygraph spec for resolving one subtag to an exposed template parameter binding.
    struct subtag_binding_spec
    {
        /// Query handled by this spec.
        using query = subtag_binding_query;
        /// Queries needed to inspect the owning instantiation's subtag bindings.
        using dependencies = rpnx::typelist< instanciation_subtag_bindings_query >;
    };

    /// Implements subtag binding lookup for one subtag symbol.
    rpnx::querygraph::coroutine< subtag_binding_spec > subtag_binding_impl(subtag_type input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_SUBTAG_BINDING_SPEC_HEADER_GUARD
