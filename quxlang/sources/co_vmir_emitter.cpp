//
// Created by Ryan Nicholl on 4/2/24.
//
#include "quxlang/manipulators/vmmanip.hpp"
#include "quxlang/res/expr/co_vmir_expression_emitter.hpp"
#include <quxlang/compiler.hpp>
#include <quxlang/data/expression.hpp>

QUX_SUBCO_MEMBER_FUNC_DEF(co_vmir_expression_emitter, emit_vm_value, quxlang::vm_value, (expression expr))
{
    if (typeis< expression_symbol_reference >(expr))
    {
        co_return co_await emit_value(as< expression_symbol_reference >(std::move(expr)));
    }
    else if (typeis< expression_binary >(expr))
    {
        co_return co_await emit_value(as< expression_binary >(std::move(expr)));
    }
    else if (typeis< expression_call >(expr))
    {
        co_return co_await emit_value(as< expression_call >(std::move(expr)));
    }
    else if (typeis< expression_numeric_literal >(expr))
    {
        co_return co_await emit_value(as< expression_numeric_literal >(std::move(expr)));
    }
    else if (typeis< expression_thisdot_reference >(expr))
    {
        co_return co_await emit_value(as< expression_thisdot_reference >(std::move(expr)));
    }
    else if (typeis< expression_this_reference >(expr))
    {
        co_return co_await emit_value(as< expression_this_reference >(std::move(expr)));
    }
    else if (typeis< expression_dotreference >(expr))
    {
        co_return co_await emit_value(as< expression_dotreference >(std::move(expr)));
    }

    else
    {
        throw std::logic_error("Unimplemented handler for " + std::string(expr.type().name()));
    }

    assert(false);
}

QUX_SUBCO_MEMBER_FUNC_DEF(co_vmir_expression_emitter, emit_value, quxlang::vm_value, (expression_symbol_reference expr))
{
    co_return (co_await inter->lookup_symbol(expr)).value();
}

QUX_SUBCO_MEMBER_FUNC_DEF(co_vmir_expression_emitter, emit_value, quxlang::vm_value, (expression_binary input))
{
    auto lhs = co_await emit_vm_value(input.lhs);
    auto rhs = co_await emit_vm_value(input.rhs);

    type_symbol lhs_type = rpnx::apply_visitor< type_symbol >(vm_value_type_vistor(), lhs);
    type_symbol rhs_type = rpnx::apply_visitor< type_symbol >(vm_value_type_vistor(), rhs);

    type_symbol lhs_underlying_type = remove_ref(lhs_type);
    type_symbol rhs_underlying_type = remove_ref(rhs_type);

    type_symbol lhs_function = subdotentity_reference{lhs_underlying_type, "OPERATOR" + input.operator_str};
    type_symbol rhs_function = subdotentity_reference{rhs_underlying_type, "OPERATOR" + input.operator_str + "RHS"};

    call_type lhs_param_info{.this_parameter = lhs_type, .positional_parameters = {rhs_type}};
    call_type rhs_param_info{.this_parameter = rhs_type, .positional_parameters = {lhs_type}};

    auto lhs_exists_and_callable_with = co_await *c->lk_functum_exists_and_is_callable_with({.functum = lhs_function, .call = lhs_param_info});

    if (lhs_exists_and_callable_with)
    {
        co_return co_await inter->invoke_functanoid(lhs_function, std::vector< vm_value >{lhs, rhs});
    }

    auto rhs_exists_and_callable_with = co_await *c->lk_functum_exists_and_is_callable_with({.functum = rhs_function, .call = rhs_param_info});

    if (rhs_exists_and_callable_with)
    {
        co_return co_await emit_call(rhs_function, {.positional = std::vector< vm_value >{rhs, lhs}});
    }

    throw std::logic_error("Found neither " + to_string(lhs_function) + " callable with (" + to_string(lhs_type) + ", " + to_string(rhs_type) + ") nor " + to_string(rhs_function) + " callable with (" + to_string(rhs_type) + ", " + to_string(lhs_type) + ")");
}
