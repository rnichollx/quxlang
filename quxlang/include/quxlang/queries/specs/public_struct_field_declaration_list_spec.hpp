// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_PUBLIC_STRUCT_FIELD_DECLARATION_LIST_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_PUBLIC_STRUCT_FIELD_DECLARATION_LIST_HEADER_GUARD

#include <quxlang/queries/declaration_privacy.hpp>
#include <quxlang/queries/public_struct_field_declaration_list.hpp>
#include <quxlang/queries/struct_field_declaration_list.hpp>
#include <quxlang/queries/symboid.hpp>
#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Dependencies for public field enumeration without field type or layout resolution. */
    struct public_struct_field_declaration_list_spec
    {
        using query = public_struct_field_declaration_list_query;
        using dependencies = rpnx::typelist< declaration_privacy_query, struct_field_declaration_list_query, symboid_query >;
    };

    /** Filters the direct field declarations using their declaration privacy. */
    rpnx::querygraph::coroutine< public_struct_field_declaration_list_spec > public_struct_field_declaration_list_impl(type_symbol input);
}

#endif // QUXLANG_QUERIES_SPECS_PUBLIC_STRUCT_FIELD_DECLARATION_LIST_HEADER_GUARD
