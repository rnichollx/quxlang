// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_INSTANCIATION_SUBTAG_BINDINGS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_INSTANCIATION_SUBTAG_BINDINGS_SPEC_HEADER_GUARD

#include <quxlang/queries/instanciation_subtag_bindings.hpp>
#include <quxlang/queries/instanciation_tempar_map.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /// Querygraph spec for collecting subtag bindings exposed by a template instantiation.
    struct instanciation_subtag_bindings_spec
    {
        /// Query handled by this spec.
        using query = instanciation_subtag_bindings_query;
        /// Queries needed to determine explicit and deduced template parameter bindings.
        using dependencies = rpnx::typelist< instanciation_tempar_map_query, symbol_type_query >;
    };

    /// Implements collection of exposed subtag bindings for one instantiation.
    rpnx::querygraph::coroutine< instanciation_subtag_bindings_spec > instanciation_subtag_bindings_impl(instanciation_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_INSTANCIATION_SUBTAG_BINDINGS_SPEC_HEADER_GUARD
