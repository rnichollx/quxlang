//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RYLANG_VM_EXPRESSION_HEADER_GUARD
#define RYLANG_VM_EXPRESSION_HEADER_GUARD

#include <boost/variant.hpp>

#include "vm_type.hpp"
namespace rylang
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

    using vm_value = boost::variant< void_value, boost::recursive_wrapper< vm_expr_primitive_binary_op >, boost::recursive_wrapper< vm_expr_primitive_unary_op >, boost::recursive_wrapper< vm_expr_load_address >, boost::recursive_wrapper< vm_expr_dereference >, boost::recursive_wrapper< vm_expr_store >, boost::recursive_wrapper< vm_expr_load_literal >, boost::recursive_wrapper< vm_expr_access_field >, boost::recursive_wrapper< vm_expr_literal >, boost::recursive_wrapper< vm_expr_call >, boost::recursive_wrapper< vm_expr_bound_value >, boost::recursive_wrapper<vm_expr_reinterpret>, boost::recursive_wrapper<vm_expr_poison> >;

} // namespace rylang

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

#endif // RYLANG_VM_EXPRESSION_HEADER_GUARD
