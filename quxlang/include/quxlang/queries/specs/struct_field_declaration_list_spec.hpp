// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_STRUCT_FIELD_DECLARATION_LIST_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_STRUCT_FIELD_DECLARATION_LIST_SPEC_HEADER_GUARD

#include <quxlang/queries/struct_field_declaration_list.hpp>
#include <quxlang/queries/class_builtin.hpp>
#include <quxlang/queries/lambda_capture_set.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/user_vm_procedure3.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct struct_field_declaration_list_spec
    {
        using query = struct_field_declaration_list_query;
        using dependencies = rpnx::typelist< class_builtin_query, lambda_capture_set_subquery, symboid_query, user_vm_procedure3_query >;
    };

    rpnx::querygraph::coroutine< struct_field_declaration_list_spec > struct_field_declaration_list_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_STRUCT_FIELD_DECLARATION_LIST_SPEC_HEADER_GUARD
