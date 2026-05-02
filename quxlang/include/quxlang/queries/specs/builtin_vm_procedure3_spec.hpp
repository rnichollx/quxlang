// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_BUILTIN_VM_PROCEDURE3_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_BUILTIN_VM_PROCEDURE3_SPEC_HEADER_GUARD

#include <quxlang/co_vmir_generator2_deps.hpp>
#include <quxlang/queries/builtin_vm_procedure3.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct builtin_vm_procedure3_spec
    {
        using query = builtin_vm_procedure3_query;
        using dependencies = co_vmir_generator2_builtin_vm_query_deps;
    };

    rpnx::querygraph::coroutine< builtin_vm_procedure3_spec > builtin_vm_procedure3_impl(instanciation_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_BUILTIN_VM_PROCEDURE3_SPEC_HEADER_GUARD
