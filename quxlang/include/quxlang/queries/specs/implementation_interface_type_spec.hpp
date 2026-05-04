// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_IMPLEMENTATION_INTERFACE_TYPE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_IMPLEMENTATION_INTERFACE_TYPE_SPEC_HEADER_GUARD

#include <quxlang/queries/implementation_interface_type.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct implementation_interface_type_spec
    {
        using query = implementation_interface_type_query;
        using dependencies = rpnx::typelist< lookup_query, symboid_query, symbol_type_query >;
    };

    rpnx::querygraph::coroutine< implementation_interface_type_spec > implementation_interface_type_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_IMPLEMENTATION_INTERFACE_TYPE_SPEC_HEADER_GUARD
