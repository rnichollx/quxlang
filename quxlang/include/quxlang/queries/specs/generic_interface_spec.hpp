// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_GENERIC_INTERFACE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_GENERIC_INTERFACE_SPEC_HEADER_GUARD

#include <quxlang/queries/generic_interface.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/symboid.hpp>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Querygraph specification for generic dispatch interface synthesis. */
    struct generic_interface_spec
    {
        using query = generic_interface_query;
        using dependencies = rpnx::typelist< lookup_query, symboid_query >;
    };

    /** Expands generic contracts and resolves their erased signatures in each declaring scope. */
    rpnx::querygraph::coroutine< generic_interface_spec > generic_interface_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_GENERIC_INTERFACE_SPEC_HEADER_GUARD
