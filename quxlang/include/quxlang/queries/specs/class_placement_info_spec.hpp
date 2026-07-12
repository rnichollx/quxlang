// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CLASS_PLACEMENT_INFO_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CLASS_PLACEMENT_INFO_SPEC_HEADER_GUARD

#include <quxlang/queries/class_placement_info.hpp>
#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/struct_layout.hpp>
#include <quxlang/queries/enum_info.hpp>
#include <quxlang/queries/flagset_info.hpp>
#include <quxlang/queries/fusion_layout.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct class_placement_info_spec
    {
        using query = class_placement_info_query;
        using dependencies = rpnx::typelist< class_type_query, struct_layout_query, enum_info_query, flagset_info_query, fusion_layout_query, machine_info_query, symbol_type_query, class_placement_info_query >;
    };

    rpnx::querygraph::coroutine< class_placement_info_spec > class_placement_info_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CLASS_PLACEMENT_INFO_SPEC_HEADER_GUARD
