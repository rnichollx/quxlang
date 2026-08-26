// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_STRUCT_MEMBER_LOOKUP_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_STRUCT_MEMBER_LOOKUP_SPEC_HEADER_GUARD

#include <quxlang/queries/active_symboid_subdeclaroids.hpp>
#include <quxlang/queries/struct_direct_bases.hpp>
#include <quxlang/queries/struct_inheritance_info.hpp>
#include <quxlang/queries/struct_member_lookup.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct struct_member_lookup_spec
    {
        using query = struct_member_lookup_query;
        using dependencies = rpnx::typelist< active_symboid_subdeclaroids_query, struct_direct_bases_query, struct_inheritance_info_query >;
    };

    /** Resolves one member name through a normalized struct hierarchy. */
    auto struct_member_lookup_impl(struct_member_lookup_input input) -> rpnx::querygraph::coroutine< struct_member_lookup_spec >;
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_STRUCT_MEMBER_LOOKUP_SPEC_HEADER_GUARD
