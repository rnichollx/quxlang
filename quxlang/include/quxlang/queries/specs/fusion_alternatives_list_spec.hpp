// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUSION_ALTERNATIVES_LIST_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUSION_ALTERNATIVES_LIST_SPEC_HEADER_GUARD

#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/fusion_alternatives_list.hpp>
#include <quxlang/queries/union_info.hpp>
#include <quxlang/queries/variant_info.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** QueryGraph specification for fusion_alternatives_list_query. */
    struct fusion_alternatives_list_spec
    {
        using query = fusion_alternatives_list_query;
        using dependencies = rpnx::typelist< class_type_query, union_info_query, variant_info_query >;
    };

    /** Collects canonical UNION or VARIANT alternative types without requesting physical layout. */
    auto fusion_alternatives_list_impl(type_symbol input) -> rpnx::querygraph::coroutine< fusion_alternatives_list_spec >;
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUSION_ALTERNATIVES_LIST_SPEC_HEADER_GUARD
