// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_STRUCT_INHERITANCE_INFO_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_STRUCT_INHERITANCE_INFO_SPEC_HEADER_GUARD

#include <quxlang/queries/struct_direct_bases.hpp>
#include <quxlang/queries/struct_inheritance_info.hpp>
#include <quxlang/queries/struct_tags.hpp>
#include <quxlang/queries/symboid.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct struct_inheritance_info_spec
    {
        using query = struct_inheritance_info_query;
        using dependencies = rpnx::typelist< struct_direct_bases_query, struct_tags_query, symboid_query >;
    };

    /** Returns the canonical inheritance graph and validates transitive hierarchy rules. */
    auto struct_inheritance_info_impl(type_symbol input) -> rpnx::querygraph::coroutine< struct_inheritance_info_spec >;
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_STRUCT_INHERITANCE_INFO_SPEC_HEADER_GUARD
