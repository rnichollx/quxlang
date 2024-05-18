//
// Created by Ryan Nicholl on 4/2/24.
//
#include "quxlang/data/expression_call.hpp"
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

QUX_SUBCO_MEMBER_FUNC_DEF(co_vmir_expression_emitter, gen_call_expr, quxlang::vm_value, (quxlang::expression_call call))
{
    auto callee = co_await emit_vm_value(call.callee);

    // TODO: Support OPERATOR() here

    if (!typeis< vm_expr_bound_value >(callee))
    {
        throw std::logic_error("Cannot call non-callable value");
    }

    auto const& callee_value = as< vm_expr_bound_value >(callee);

    vm_callargs args;

    call_type call_t;

    for (auto& arg : call.args)
    {
        auto arg_val = co_await emit_vm_value(arg.value);

        if (arg.name)
        {
            args.named[*arg.name] = arg_val;
            call_t.named_parameters[*arg.name] = vm_value_type(arg_val);
        }
        else
        {
            args.positional.push_back(arg_val);
            call_t.positional_parameters.push_back(vm_value_type(arg_val));
        }
    }

    if (!callee_value.value.type_is< void_value >())
    {
        call_t.named_parameters["THIS"] = vm_value_type(callee_value.value);
    }

    co_return co_await gen_call_functum(callee_value.function_ref, args);
}

QUX_SUBCO_MEMBER_FUNC_DEF(co_vmir_expression_emitter, gen_call_functum, quxlang::vm_value, (type_symbol func, vm_callargs args))
{
    call_type calltype;
    for (auto& arg : args.positional)
    {
        calltype.positional_parameters.push_back(vm_value_type(arg));
    }
    for (auto& [name, arg] : args.named)
    {
        calltype.named_parameters[name] = vm_value_type(arg);
    }

    instanciation_reference functanoid_unnormalized{.callee = func, .parameters = calltype};
    // Get call type
    auto instanciation = QUX_CO_ASK(instanciation, (functanoid_unnormalized));
    if (!instanciation)
    {
        throw std::logic_error("Cannot call " + to_string(func) + " with " + to_string(calltype));
    }
    co_return co_await gen_call_functanoid(instanciation.value(), args);
}

QUX_SUBCO_MEMBER_FUNC_DEF(co_vmir_expression_emitter, gen_call_functanoid, quxlang::vm_value, (instanciation_reference what, vm_callargs args))
{
    auto const& call_args_types = what.parameters;

    // TODO: Support defaulted parameters.

    std::vector< co_interface::deferral_index > deferrals;

    vm_invocation_args invocation_args;

    for (auto const& [name, arg_accepted_type] : call_args_types.named_parameters)
    {

        auto arg_expr = args.named.at(name);
        auto arg_expr_type = vm_value_type(arg_expr);

        // TODO: Support PRValue args
        assert(is_ref(arg_expr_type));

        if (!is_ref(arg_accepted_type))
        {
            auto index = co_await inter->create_temporary_storage(arg_accepted_type);


            auto arg_final_ctor_func = subdotentity_reference{arg_accepted_type, "CONSTRUCTOR"};
            auto arg_final_dtor_func = subdotentity_reference{arg_accepted_type, "DESTRUCTOR"};

            vm_callargs ctor_args = {.named = {{"THIS", vm_expr_load_reference{.index= index, .type=make_mref(arg_accepted_type)}}}, .positional = {arg_expr}};

            // These both need to be references or the constructor will probably infinite loop.
            assert(is_ref(arg_expr_type) && is_ref(make_mref(arg_accepted_type)));

            // TODO: instead of directly calling the constructor, call a special conversion function perhaps?
            co_await gen_call_functum(arg_final_ctor_func, ctor_args);

            // When we complete the constructor of the argument, we need to queue the destructor of the argument.
            // We also need to save the deferral so we can remove it from the deferral list prior to invoking
            // the target procedure (since ownership of the argument is transferred to the target procedure).

            invocation_args.named[name] = index;

            vm_invocation_args dtor_args = {.named = {{"THIS", index}}};

            co_interface::deferral_index defer_index = co_await inter->defer(arg_dtor_functum, dtor_args);
            deferrals.push_back(defer_index);
        }
        else
        {
            throw rpnx::unimplemented();
        }
    }
}

QUX_SUBCO_MEMBER_FUNC_DEF(co_vmir_expression_emitter, emit_value, quxlang::vm_value, (expression_symbol_reference expr))
{
    co_return (co_await inter->lookup_symbol(expr.symbol)).value();
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
    call_type lhs_param_info{.named_parameters = {{"THIS", lhs_type}}, .positional_parameters = {rhs_type}};
    call_type rhs_param_info{.named_parameters = {{"THIS", rhs_type}}, .positional_parameters = {lhs_type}};

    auto lhs_exists_and_callable_with = co_await *c->lk_functum_exists_and_is_callable_with({.callee = lhs_function, .parameters = lhs_param_info});

    if (lhs_exists_and_callable_with)
    {
        auto lhs_args = vm_callargs{.named = {{"THIS", lhs}}, .positional = {rhs}};
        co_return co_await gen_call_functanoid(lhs_function, lhs_args);
    }

    auto rhs_exists_and_callable_with = co_await *c->lk_functum_exists_and_is_callable_with({.callee = rhs_function, .parameters = rhs_param_info});

    if (rhs_exists_and_callable_with)
    {
        auto rhs_args = vm_callargs{.named = {{"THIS", rhs}}, .positional = {lhs}};
        co_return co_await gen_call_functanoid(rhs_function, rhs_args);
    }

    throw std::logic_error("Found neither " + to_string(lhs_function) + " callable with (" + to_string(lhs_type) + ", " + to_string(rhs_type) + ") nor " + to_string(rhs_function) + " callable with (" + to_string(rhs_type) + ", " + to_string(lhs_type) + ")");
}
