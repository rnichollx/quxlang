// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CLASS_FIELD_LIST_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CLASS_FIELD_LIST_SPEC_HEADER_GUARD

#include <quxlang/queries/class_field_list.hpp>
#include <quxlang/queries/class_field_declaration_list.hpp>
#include <quxlang/queries/lookup.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using class_field_list_spec = rpnx::querygraph::query_handler_spec< class_field_list_query, rpnx::typelist< class_field_declaration_list_query, lookup_query > >;

    rpnx::querygraph::coroutine< class_field_list_spec > class_field_list_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CLASS_FIELD_LIST_SPEC_HEADER_GUARD
