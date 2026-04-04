// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CLASS_LAYOUT_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CLASS_LAYOUT_SPEC_HEADER_GUARD

#include <quxlang/queries/class_layout.hpp>
#include <quxlang/queries/class_field_list.hpp>
#include <quxlang/queries/type_placement_info.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using class_layout_spec = rpnx::query_handler_spec< class_layout_query, rpnx::typelist< class_field_list_query, type_placement_info_query > >;

    rpnx::querygraph::coroutine< class_layout_spec > class_layout_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CLASS_LAYOUT_SPEC_HEADER_GUARD
