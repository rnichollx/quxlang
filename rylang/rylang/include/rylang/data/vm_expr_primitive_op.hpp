//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_EXPR_PRIMITIVE_OP_HEADER
#define RPNX_RYANSCRIPT1031_VM_EXPR_PRIMITIVE_OP_HEADER

#include "vm_expression.hpp"
namespace rylang
{
    enum class vm_primitive_binary_operator {
        add,
        sub,
        mul,
        div,
        mod,
        bitand_,
        bitor_,
        bitxor,
        logicand,
        logicor,
        logicxor,
        shiftleft,
        shiftright,
        equals,
        notequals,
        greater,
        less,
        greaterequals,
        lessequals
    };

    enum class vm_primitive_unary_operator { negate, bitflip, logicnot };

    struct vm_expr_primitive_binary_op
    {
        vm_value lhs;
        vm_value rhs;
        vm_primitive_binary_operator oper;
        qualified_symbol_reference type;
    };

    struct vm_expr_primitive_unary_op
    {
        vm_value expr;
        vm_primitive_unary_operator oper;
        qualified_symbol_reference type;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_EXPR_PRIMITIVE_OP_HEADER
