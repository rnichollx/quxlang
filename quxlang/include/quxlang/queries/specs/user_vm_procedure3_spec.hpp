// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_USER_VM_PROCEDURE3_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_USER_VM_PROCEDURE3_SPEC_HEADER_GUARD

#include <quxlang/co_vmir_generator2_deps.hpp>
#include <quxlang/queries/functanoid_deduced_return_type.hpp>
#include <quxlang/queries/user_vm_procedure3.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct user_vm_procedure3_spec
    {
        using query = user_vm_procedure3_query;
        using dependencies = co_vmir_generator2_query_deps;
        using produced_subqueries = rpnx::typelist< functanoid_deduced_return_type >;
    };

    rpnx::querygraph::coroutine< user_vm_procedure3_spec > user_vm_procedure3_impl(instanciation_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_USER_VM_PROCEDURE3_SPEC_HEADER_GUARD
