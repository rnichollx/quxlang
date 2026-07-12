// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUSION_LAYOUT_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUSION_LAYOUT_SPEC_HEADER_GUARD

#include <quxlang/queries/class_placement_info.hpp>
#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/fusion_layout.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/union_info.hpp>
#include <quxlang/queries/variant_info.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /// QueryGraph specification for fusion_layout_query.
    struct fusion_layout_spec
    {
        using query = fusion_layout_query;
        using dependencies = rpnx::typelist< class_placement_info_query, class_type_query, machine_info_query, union_info_query, variant_info_query >;
    };

    /// Computes the target-specific layout of one fusion type.
    rpnx::querygraph::coroutine< fusion_layout_spec > fusion_layout_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUSION_LAYOUT_SPEC_HEADER_GUARD
