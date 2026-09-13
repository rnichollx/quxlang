// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_SPECS_VM_PROCEDURE3_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_VM_PROCEDURE3_SPEC_HEADER_GUARD
#include <rpnx/querygraph/querygraph.hpp>
#include <quxlang/co_vmir_generator2_deps.hpp>
#include <quxlang/queries/builtin_vm_procedure3.hpp>
#include <quxlang/queries/lambda_possible_captures.hpp>
#include <quxlang/queries/lambda_capture_set.hpp>
#include <quxlang/queries/lambda_environment.hpp>
#include <quxlang/queries/lambda_operator.hpp>
#include <quxlang/queries/functanoid_deduced_return_type.hpp>
namespace quxlang
{
    /// VM generation and its progressively published lexical declarations.
    struct vm_procedure3_spec
    {
        using query = vm_procedure3_query;
        using dependencies = co_vmir_generator2_query_deps;
        using produced_subqueries = rpnx::typelist< body_parent, published_name_info, functanoid_deduced_return_type, lambda_possible_captures_subquery, lambda_capture_set_subquery, lambda_environment_subquery, lambda_operator_subquery >;
    };
    rpnx::querygraph::coroutine< vm_procedure3_spec > vm_procedure3_impl(instanciation_reference input);
}
#endif
