// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TYPE_PLACEMENT_INFO_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TYPE_PLACEMENT_INFO_SPEC_HEADER_GUARD

#include <quxlang/queries/type_placement_info.hpp>
#include <quxlang/queries/class_layout.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct type_placement_info_spec
    {
        using query = type_placement_info_query;
        using dependencies = rpnx::typelist< class_layout_query, machine_info_query, symbol_type_query, type_placement_info_query >;
    };

    rpnx::querygraph::coroutine< type_placement_info_spec > type_placement_info_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TYPE_PLACEMENT_INFO_SPEC_HEADER_GUARD
