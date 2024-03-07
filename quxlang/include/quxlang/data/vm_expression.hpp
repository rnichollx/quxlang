//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef QUXLANG_VM_EXPRESSION_HEADER_GUARD
#define QUXLANG_VM_EXPRESSION_HEADER_GUARD

#include <rpnx/variant.hpp>

#include "vm_type.hpp"

namespace quxlang
{
    struct vm_expr_load_address;
    struct vm_expr_primitive_binary_op;
    struct vm_expr_primitive_unary_op;
    struct vm_expr_dereference;
    struct vm_expr_store;
    struct vm_expr_call;
    struct vm_expr_load_literal;
    struct vm_expr_literal;

    struct void_value
    {
        std::strong_ordering operator<=>(const void_value&) const = default;
    };

    struct vm_expr_bound_value;
    // struct vm_expr_call;

    struct vm_expr_access_field;

    struct vm_expr_reinterpret;

    struct vm_expr_poison;

    using vm_value = rpnx::variant< void_value, vm_expr_primitive_binary_op, vm_expr_primitive_unary_op, vm_expr_load_address, vm_expr_dereference, vm_expr_store, vm_expr_load_literal, vm_expr_access_field, vm_expr_literal, vm_expr_call, vm_expr_bound_value, vm_expr_reinterpret, vm_expr_poison >;

} // namespace quxlang

#include "vm_expr_access_field.hpp"
#include "vm_expr_bound_value.hpp"
#include "vm_expr_call.hpp"
#include "vm_expr_dereference.hpp"
#include "vm_expr_load_address.hpp"
#include "vm_expr_load_literal.hpp"
#include "vm_expr_primitive_op.hpp"
#include "vm_expr_store.hpp"
#include "vm_expr_reinterpret.hpp"
#include "vm_expr_poison.hpp"
#include "vm_expr_undef.hpp"

#endif // QUXLANG_VM_EXPRESSION_HEADER_GUARD