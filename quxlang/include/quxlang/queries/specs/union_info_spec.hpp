// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_UNION_INFO_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_UNION_INFO_SPEC_HEADER_GUARD

#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/union_info.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /// QueryGraph specification for union_info_query.
    struct union_info_spec
    {
        using query = union_info_query;
        using dependencies = rpnx::typelist< lookup_query, symboid_query >;
    };

    /// Normalizes and validates one UNION declaration.
    rpnx::querygraph::coroutine< union_info_spec > union_info_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_UNION_INFO_SPEC_HEADER_GUARD
