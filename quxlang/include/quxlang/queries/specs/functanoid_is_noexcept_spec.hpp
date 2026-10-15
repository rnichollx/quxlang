// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_SPECS_FUNCTANOID_IS_NOEXCEPT_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTANOID_IS_NOEXCEPT_HEADER_GUARD
#include <quxlang/queries/functanoid_is_noexcept.hpp>
#include <quxlang/queries/functum_initialize.hpp>
#include <quxlang/queries/user_default_dtor_exists.hpp>
#include <quxlang/queries/function_declaration.hpp>
#include <quxlang/queries/constexpr_bool.hpp>
#include <quxlang/queries/function_builtin.hpp>
#include <quxlang/queries/builtin_vm_procedure3.hpp>
#include <rpnx/querygraph/querygraph.hpp>
namespace quxlang
{
    /** Declares dependencies for the exception-contract query. */
    struct functanoid_is_noexcept_spec
    {
        using query = functanoid_is_noexcept_query;
        using dependencies = rpnx::typelist< user_default_dtor_exists_query, functum_initialize_query, function_declaration_query, constexpr_bool_query, function_builtin_query, builtin_vm_procedure3_query, functanoid_is_noexcept_query >;
    };
    /** Reports the exception contract of a functanoid. */
    rpnx::querygraph::coroutine< functanoid_is_noexcept_spec > functanoid_is_noexcept_impl(instanciation_reference input);
}
#endif
