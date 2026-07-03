// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_GLOBAL_IS_NUMERIC_STATIC_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_GLOBAL_IS_NUMERIC_STATIC_SPEC_HEADER_GUARD

#include <quxlang/queries/global_is_numeric_static.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/variable_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct global_is_numeric_static_spec
    {
        using query = global_is_numeric_static_query;
        using dependencies = rpnx::typelist< symboid_query, symbol_type_query, variable_type_query >;
    };

    rpnx::querygraph::coroutine< global_is_numeric_static_spec > global_is_numeric_static_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_GLOBAL_IS_NUMERIC_STATIC_SPEC_HEADER_GUARD
