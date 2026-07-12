// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_VARIANT_INFO_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_VARIANT_INFO_SPEC_HEADER_GUARD

#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/variant_info.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /// QueryGraph specification for variant_info_query.
    struct variant_info_spec
    {
        using query = variant_info_query;
        using dependencies = rpnx::typelist< lookup_query, symboid_query >;
    };

    /// Normalizes and validates one VARIANT declaration.
    rpnx::querygraph::coroutine< variant_info_spec > variant_info_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_VARIANT_INFO_SPEC_HEADER_GUARD
