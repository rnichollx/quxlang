// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_VM_PROCEDURE3_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_VM_PROCEDURE3_SPEC_HEADER_GUARD

#include <quxlang/queries/vm_procedure3.hpp>
#include <quxlang/queries/builtin_vm_procedure3.hpp>
#include <quxlang/queries/function_builtin.hpp>
#include <quxlang/queries/user_vm_procedure3.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using vm_procedure3_spec = rpnx::querygraph::query_handler_spec< vm_procedure3_query, rpnx::typelist< builtin_vm_procedure3_query, function_builtin_query, user_vm_procedure3_query > >;

    rpnx::querygraph::coroutine< vm_procedure3_spec > vm_procedure3_impl(instanciation_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_VM_PROCEDURE3_SPEC_HEADER_GUARD
