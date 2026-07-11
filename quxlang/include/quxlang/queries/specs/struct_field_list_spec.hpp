// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_STRUCT_FIELD_LIST_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_STRUCT_FIELD_LIST_SPEC_HEADER_GUARD

#include <quxlang/queries/struct_field_list.hpp>
#include <quxlang/queries/struct_field_declaration_list.hpp>
#include <quxlang/queries/lookup.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct struct_field_list_spec
    {
        using query = struct_field_list_query;
        using dependencies = rpnx::typelist< struct_field_declaration_list_query, lookup_query >;
    };

    rpnx::querygraph::coroutine< struct_field_list_spec > struct_field_list_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_STRUCT_FIELD_LIST_SPEC_HEADER_GUARD
