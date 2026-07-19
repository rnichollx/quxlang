// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_STRUCT_LAYOUT_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_STRUCT_LAYOUT_SPEC_HEADER_GUARD

#include <quxlang/queries/struct_layout.hpp>
#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/struct_field_list.hpp>
#include <quxlang/queries/class_placement_info.hpp>
#include <quxlang/queries/symboid.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct struct_layout_spec
    {
        using query = struct_layout_query;
        using dependencies = rpnx::typelist< class_type_query, struct_field_list_query, class_placement_info_query, symboid_query >;
    };

    rpnx::querygraph::coroutine< struct_layout_spec > struct_layout_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_STRUCT_LAYOUT_SPEC_HEADER_GUARD
