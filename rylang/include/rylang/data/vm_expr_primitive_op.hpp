//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RYLANG_VM_EXPR_PRIMITIVE_OP_HEADER_GUARD
#define RYLANG_VM_EXPR_PRIMITIVE_OP_HEADER_GUARD

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
        std::string oper;
        type_symbol type;
    };

    struct vm_expr_primitive_unary_op
    {
        vm_value expr;
        vm_primitive_unary_operator oper;
        type_symbol type;
    };
} // namespace rylang

#endif // RYLANG_VM_EXPR_PRIMITIVE_OP_HEADER_GUARD
