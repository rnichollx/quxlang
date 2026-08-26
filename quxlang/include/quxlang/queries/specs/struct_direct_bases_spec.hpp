// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_STRUCT_DIRECT_BASES_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_STRUCT_DIRECT_BASES_SPEC_HEADER_GUARD

#include <quxlang/queries/active_symboid_subdeclaroids.hpp>
#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/struct_direct_bases.hpp>
#include <quxlang/queries/struct_tags.hpp>
#include <quxlang/queries/symboid.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct struct_direct_bases_spec
    {
        using query = struct_direct_bases_query;
        using dependencies = rpnx::typelist< active_symboid_subdeclaroids_query, class_type_query, lookup_query, struct_tags_query, symboid_query >;
    };

    /** Resolves and validates the active direct bases declared by one struct. */
    auto struct_direct_bases_impl(type_symbol input) -> rpnx::querygraph::coroutine< struct_direct_bases_spec >;
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_STRUCT_DIRECT_BASES_SPEC_HEADER_GUARD
