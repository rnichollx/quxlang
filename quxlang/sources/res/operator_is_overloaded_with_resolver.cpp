//
// Created by Ryan Nicholl on 11/11/23.
//
#include "quxlang/compiler.hpp"

void quxlang::operator_is_overloaded_with_resolver::process(compiler* c)
{
    // TODO: check if the operator is overloaded

    auto lhs_type = m_lhs;
    auto rhs_type = m_rhs;
    auto op = m_op;

    auto lhs_underlying_type = remove_ref(lhs_type);
    auto rhs_underlying_type = remove_ref(rhs_type);

    call_parameter_information cs1;
    cs1.argument_types = {lhs_type, rhs_type};
    auto operator1 = subdotentity_reference{lhs_underlying_type, "OPERATOR" + op};

    call_parameter_information cs2;
    cs2.argument_types = {rhs_type, lhs_type};
    auto operator2 = subdotentity_reference{rhs_underlying_type, "OPERATOR" + op + "RHS"};

    if (is_integral(lhs_underlying_type) && is_integral(rhs_underlying_type))
    {
        set_value(subdotentity_reference{lhs_underlying_type, "OPERATOR" + op });
        return;
    }

    if (is_integral(lhs_underlying_type) && is_numeric_literal(rhs_underlying_type))
    {
        set_value(subdotentity_reference{lhs_underlying_type, "OPERATOR" + op });
        return;
    }

    auto op1_exists_dp = get_dependency(
        [&]
        {
            return c->lk_symbol_canonical_chain_exists(operator1);
        });

    auto op2_exists_dp = get_dependency(
        [&]
        {
            return c->lk_symbol_canonical_chain_exists(operator2);
        });

    if (!ready())
    {
        return;
    }

    set_value(std::nullopt);
}
