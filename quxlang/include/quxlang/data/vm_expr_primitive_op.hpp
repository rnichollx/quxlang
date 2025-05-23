// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_VM_EXPR_PRIMITIVE_OP_HEADER_GUARD
#define QUXLANG_DATA_VM_EXPR_PRIMITIVE_OP_HEADER_GUARD

#include "vm_expression.hpp"
namespace quxlang
{
    enum class vm_primitive_binary_operator : std::uint16_t { add, sub, mul, div, mod, bitand_, bitor_, bitxor, logicand, logicor, logicxor, shiftleft, shiftright, equals, notequals, greater, less, greaterequals, lessequals };
}
RPNX_ENUM(quxlang, vm_primitive_unary_operator, std::uint16_t, negate, bitflip, logicnot);

RPNX_ENUM(quxlang, vm_primitive_binary_operator2, std::uint16_t, sadd, uadd, ssub, usub, smul);

namespace quxlang
{
    struct vm_expr_primitive_binary_op
    {
        vm_value lhs;
        vm_value rhs;
        std::string oper;
        type_symbol type;

        RPNX_MEMBER_METADATA(vm_expr_primitive_binary_op, lhs, rhs, oper, type);
    };

    struct vm_expr_primitive_binary_op2
    {
        vm_primitive_binary_operator2 oper;
        std::size_t result;
        std::size_t lhs;
        std::size_t rhs;

        RPNX_MEMBER_METADATA(vm_expr_primitive_binary_op2, oper, result, lhs, rhs);
    };

    struct vm_expr_primitive_unary_op
    {
        vm_value expr;
        vm_primitive_unary_operator oper;
        type_symbol type;

        RPNX_MEMBER_METADATA(vm_expr_primitive_unary_op, expr, oper, type);
    };
} // namespace quxlang

#endif // QUXLANG_VM_EXPR_PRIMITIVE_OP_HEADER_GUARD
