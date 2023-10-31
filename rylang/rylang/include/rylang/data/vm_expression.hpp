//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_EXPRESSION_HEADER
#define RPNX_RYANSCRIPT1031_VM_EXPRESSION_HEADER

#include <boost/variant.hpp>

#include "vm_type.hpp"
namespace rylang
{
    struct vm_expr_primitive_op;
    struct vm_expr_load_address;
    struct vm_expr_call;

    using vm_expression = boost::variant< boost::recursive_wrapper< vm_expr_primitive_op >, boost::recursive_wrapper< vm_expr_load_address >, boost::recursive_wrapper< vm_expr_call > >;
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_EXPRESSION_HEADER
