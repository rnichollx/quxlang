// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_VARIABLE_TYPE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_VARIABLE_TYPE_SPEC_HEADER_GUARD

#include <quxlang/queries/variable_type.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/symboid.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct variable_type_spec
    {
        using query = variable_type_query;
        using dependencies = rpnx::typelist< lookup_query, symboid_query >;
    };

    rpnx::querygraph::coroutine< variable_type_spec > variable_type_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_VARIABLE_TYPE_SPEC_HEADER_GUARD
