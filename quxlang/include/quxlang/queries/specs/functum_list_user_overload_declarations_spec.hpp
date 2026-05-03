// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTUM_LIST_USER_OVERLOAD_DECLARATIONS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTUM_LIST_USER_OVERLOAD_DECLARATIONS_SPEC_HEADER_GUARD

#include <quxlang/queries/functum_list_user_overload_declarations.hpp>
#include <quxlang/queries/lambda_capture_set.hpp>
#include <quxlang/queries/lambda_operator.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/user_vm_procedure3.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct functum_list_user_overload_declarations_spec
    {
        using query = functum_list_user_overload_declarations_query;
        using dependencies = rpnx::typelist< lambda_capture_set_subquery, lambda_operator_subquery, symboid_query, user_vm_procedure3_query >;
    };

    rpnx::querygraph::coroutine< functum_list_user_overload_declarations_spec > functum_list_user_overload_declarations_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTUM_LIST_USER_OVERLOAD_DECLARATIONS_SPEC_HEADER_GUARD
