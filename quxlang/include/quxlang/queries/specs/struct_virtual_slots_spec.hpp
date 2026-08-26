// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_STRUCT_VIRTUAL_SLOTS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_STRUCT_VIRTUAL_SLOTS_SPEC_HEADER_GUARD

#include <quxlang/queries/active_symboid_subdeclaroids.hpp>
#include <quxlang/queries/constexpr_bool.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/struct_direct_bases.hpp>
#include <quxlang/queries/struct_inheritance_info.hpp>
#include <quxlang/queries/struct_tags.hpp>
#include <quxlang/queries/struct_virtual_slots.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct struct_virtual_slots_spec
    {
        using query = struct_virtual_slots_query;
        using dependencies = rpnx::typelist< active_symboid_subdeclaroids_query, constexpr_bool_query, lookup_query, struct_direct_bases_query, struct_inheritance_info_query, struct_tags_query >;
    };

    /** Normalizes virtual declarations, overrides, and implicit destructor slots. */
    auto struct_virtual_slots_impl(type_symbol input) -> rpnx::querygraph::coroutine< struct_virtual_slots_spec >;
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_STRUCT_VIRTUAL_SLOTS_SPEC_HEADER_GUARD
