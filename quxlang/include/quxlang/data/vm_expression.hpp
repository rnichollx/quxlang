// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_VM_EXPRESSION_HEADER_GUARD
#define QUXLANG_DATA_VM_EXPRESSION_HEADER_GUARD

#include <rpnx/variant.hpp>
#include <rpnx/metadata.hpp>

#include <map>

#include "vm_type.hpp"

namespace quxlang
{
    struct vm_expr_load_reference;
    struct vm_expr_primitive_binary_op;
    struct vm_expr_primitive_unary_op;
    struct vm_expr_dereference;
    struct vm_expr_store;
    struct vm_invoke;
    struct vm_expr_load_literal;
    struct vm_expr_literal;

    struct void_value
    {
        RPNX_MEMBER_METADATA(void_value);
    };

    struct vm_expr_bound_value;
    // struct vm_expr_call;

    struct vm_expr_access_field;

    struct vm_expr_reinterpret;

    struct vm_expr_poison;

    using vm_value = rpnx::variant< void_value, vm_expr_primitive_binary_op, vm_expr_primitive_unary_op, vm_expr_load_reference, vm_expr_dereference, vm_expr_store, vm_expr_load_literal, vm_expr_access_field, vm_expr_literal, vm_expr_bound_value, vm_expr_reinterpret, vm_expr_poison >;

    struct vm_callargs
    {
        std::map< std::string, vm_value > named;

        std::vector< vm_value > positional;

        RPNX_MEMBER_METADATA(vm_callargs, named, positional);
    };

    struct vm_invocation_args
    {
        std::map<std::string, std::size_t> named;
        std::vector< std::size_t > positional;

        RPNX_MEMBER_METADATA(vm_invocation_args, named, positional);
    };

} // namespace quxlang

#include "vm_expr_access_field.hpp"
#include "vm_expr_bound_value.hpp"
#include "vm_expr_dereference.hpp"
#include "vm_expr_load_literal.hpp"
#include "vm_expr_load_reference.hpp"
#include "vm_expr_poison.hpp"
#include "vm_expr_primitive_op.hpp"
#include "vm_expr_reinterpret.hpp"
#include "vm_expr_store.hpp"
#include "vm_expr_undef.hpp"
#include "vm_invoke.hpp"

#endif // QUXLANG_VM_EXPRESSION_HEADER_GUARD