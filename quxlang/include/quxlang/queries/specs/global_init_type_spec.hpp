// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_GLOBAL_INIT_TYPE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_GLOBAL_INIT_TYPE_SPEC_HEADER_GUARD

#include <quxlang/queries/global_init_type.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/type_is_trivially_default_constructible.hpp>
#include <quxlang/queries/variable_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct global_init_type_spec
    {
        using query = global_init_type_query;
        using dependencies = rpnx::typelist< symboid_query, symbol_type_query, type_is_trivially_default_constructible_query, variable_type_query >;
    };

    /// Returns the initialization strategy used by a global variable's GET_REFERENCE builtin.
    rpnx::querygraph::coroutine< global_init_type_spec > global_init_type_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_GLOBAL_INIT_TYPE_SPEC_HEADER_GUARD
