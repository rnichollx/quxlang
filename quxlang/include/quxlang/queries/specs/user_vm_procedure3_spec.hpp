// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_USER_VM_PROCEDURE3_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_USER_VM_PROCEDURE3_SPEC_HEADER_GUARD

#include <quxlang/co_vmir_generator2_deps.hpp>
#include <quxlang/queries/functanoid_deduced_return_type.hpp>
#include <quxlang/queries/lambda_capture_set.hpp>
#include <quxlang/queries/lambda_environment.hpp>
#include <quxlang/queries/lambda_operator.hpp>
#include <quxlang/queries/lambda_possible_captures.hpp>
#include <quxlang/queries/user_vm_procedure3.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    template < typename List, typename... Extra >
    struct append_user_vm_dependencies;

    template < typename... Existing, typename... Extra >
    struct append_user_vm_dependencies< rpnx::typelist< Existing... >, Extra... >
    {
        using type = rpnx::typelist< Existing..., Extra... >;
    };

    struct user_vm_procedure3_spec
    {
        using query = user_vm_procedure3_query;
        using dependencies = typename append_user_vm_dependencies< co_vmir_generator2_query_deps, lambda_capture_set_subquery, lambda_environment_subquery, user_vm_procedure3_query >::type;
        using produced_subqueries = rpnx::typelist< functanoid_deduced_return_type, lambda_possible_captures_subquery, lambda_capture_set_subquery, lambda_environment_subquery, lambda_operator_subquery >;
    };

    rpnx::querygraph::coroutine< user_vm_procedure3_spec > user_vm_procedure3_impl(instanciation_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_USER_VM_PROCEDURE3_SPEC_HEADER_GUARD
