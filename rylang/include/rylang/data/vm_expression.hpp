//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_EXPRESSION_HEADER
#define RPNX_RYANSCRIPT1031_VM_EXPRESSION_HEADER

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
    struct void_value
    {
        std::strong_ordering operator<=>(const void_value&) const = default;
    };
    // struct vm_expr_call;

    using vm_value = boost::variant< void_value, boost::recursive_wrapper< vm_expr_primitive_binary_op >, boost::recursive_wrapper< vm_expr_primitive_unary_op >,
                                     boost::recursive_wrapper< vm_expr_load_address >, boost::recursive_wrapper< vm_expr_dereference >, boost::recursive_wrapper< vm_expr_store >,
                                     boost::recursive_wrapper< vm_expr_call > >;

} // namespace rylang

#include "vm_expr_call.hpp"
#include "vm_expr_dereference.hpp"
#include "vm_expr_load_address.hpp"
#include "vm_expr_primitive_op.hpp"
#include "vm_expr_store.hpp"

#endif // RPNX_RYANSCRIPT1031_VM_EXPRESSION_HEADER
