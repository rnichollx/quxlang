//
// Created by Ryan Nicholl on 4/2/24.
//
#include "quxlang/data/expression_call.hpp"
#include "quxlang/manipulators/vmmanip.hpp"
#include "quxlang/res/expr/co_vmir_expression_emitter.hpp"
#include <quxlang/compiler.hpp>
#include <quxlang/data/expression.hpp>

QUX_SUBCO_MEMBER_FUNC_DEF(co_vmir_expression_emitter, typeof_vm_value, quxlang::type_symbol, (expression expr))
{
    rpnx::unimplemented();
    return {};
}

QUX_SUBCO_MEMBER_FUNC_DEF(co_vmir_expression_emitter, generate_expr, quxlang::vmir2::storage_index, (expression expr))
{
    if (typeis< expression_symbol_reference >(expr))
    {
        co_return co_await generate(as< expression_symbol_reference >(std::move(expr)));
    }
    else if (typeis< expression_binary >(expr))
    {
        co_return co_await generate(as< expression_binary >(std::move(expr)));
    }
    else if (typeis< expression_call >(expr))
    {
        co_return co_await gen_call_expr(as< expression_call >(std::move(expr)));
    }
    else if (typeis< expression_numeric_literal >(expr))
    {
        co_return co_await generate(as< expression_numeric_literal >(std::move(expr)));
    }
    else if (typeis< expression_thisdot_reference >(expr))
    {
        co_return co_await generate(as< expression_thisdot_reference >(std::move(expr)));
    }
    else if (typeis< expression_this_reference >(expr))
    {
        co_return co_await generate(as< expression_this_reference >(std::move(expr)));
    }
    else if (typeis< expression_dotreference >(expr))
    {
        co_return co_await generate(as< expression_dotreference >(std::move(expr)));
    }

    else
    {
        throw std::logic_error("Unimplemented handler for " + std::string(expr.type().name()));
    }

    assert(false);
}

QUX_SUBCO_MEMBER_FUNC_DEF(co_vmir_expression_emitter, generate, quxlang::vmir2::storage_index, (expression_thisdot_reference what))
{
    auto this_reference = subdotentity_reference{.parent = context_reference{}, .subdotentity_name = "THIS"};
    auto value = co_await inter->lookup_symbol(this_reference);
    if (!value)
    {
        throw std::logic_error("Cannot find " + to_string(this_reference));
    }
    co_return value.value();
}

QUX_SUBCO_MEMBER_FUNC_DEF(co_vmir_expression_emitter, generate, quxlang::vmir2::storage_index, (expression_dotreference what))
{
    auto parent = co_await generate_expr(what.lhs);

    co_return co_await generate_field_access(parent, what.field_name);
}

QUX_SUBCO_MEMBER_FUNC_DEF(co_vmir_expression_emitter, generate_field_access, quxlang::vmir2::storage_index, (storage_index base, std::string field_name))
{
    auto base_type = co_await inter->index_type(base);
    auto base_type_noref = quxlang::remove_ref(base_type);

    class_layout layout = co_await *c->lk_class_layout_from_canonical_chain(base_type_noref);

    for (class_field_info const& field : layout.fields)
    {
        if (field.name == field_name)
        {
            vmir2::access_field access;
            access.base_index = base;
            access.offset = field.offset;
            type_symbol result_ref_type = recast_reference(base_type, field.type);
            access.store_index = co_await inter->create_temporary_storage(result_ref_type);

            co_await inter->emit(access);
            co_return access.store_index;
        }
    }

    throw std::logic_error("Cannot find field " + field_name + " in " + to_string(base_type));
}

QUX_SUBCO_MEMBER_FUNC_DEF(co_vmir_expression_emitter, gen_call_expr, quxlang::vmir2::storage_index, (quxlang::expression_call call))
{
    auto callee = co_await generate_expr(call.callee);

    type_symbol callee_type = co_await inter->index_type(callee);

    if (!typeis< bound_function_type_reference >(callee_type))
    {
        // TODO: Call OPERATOR() here instead of throwing
        throw std::logic_error("Cannot call non-callable value");
    }

    auto const& callee_type_value = as< bound_function_type_reference >(callee_type);

    vmir2::invocation_args args;

    call_type call_t;

    for (auto& arg : call.args)
    {
        auto arg_val_idx = co_await generate_expr(arg.value);

        if (arg.name)
        {
            args.named[*arg.name] = arg_val_idx;
            call_t.named_parameters[*arg.name] = co_await inter->index_type(arg_val_idx);
        }
        else
        {
            args.positional.push_back(arg_val_idx);
            call_t.positional_parameters.push_back(co_await inter->index_type(arg_val_idx));
        }
    }

    if (!typeis<void_type>(callee_type_value.object_type))
    {
        call_t.named_parameters["THIS"] = callee_type;
        args.named["THIS"] = callee;
    }

    co_return co_await gen_call_functum(callee_type_value.function_type, args);
}

QUX_SUBCO_MEMBER_FUNC_DEF(co_vmir_expression_emitter, gen_call_functum, quxlang::vmir2::storage_index, (type_symbol func, vmir2::invocation_args args))
{
    call_type calltype;
    for (auto& arg : args.positional)
    {
        calltype.positional_parameters.push_back(co_await inter->index_type(arg));
    }
    for (auto& [name, arg] : args.named)
    {
        calltype.named_parameters[name] = co_await inter->index_type(arg);
    }

    instanciation_reference functanoid_unnormalized{.callee = func, .parameters = calltype};
    // Get call type
    auto instanciation = QUX_CO_ASK(instanciation, (functanoid_unnormalized));
    if (!instanciation)
    {
        throw std::logic_error("Cannot call " + to_string(func) + " with " + to_string(calltype));
    }

    auto return_type = QUX_CO_ASK(functanoid_return_type, (instanciation.value()));

    // Index 0 is defined to be the special "void" value.
    vmir2::storage_index return_value = 0;

    if (!typeis< void_type >(return_type))
    {
        auto return_slot_type = nvalue_slot{.target = return_type};
        auto return_slot = co_await inter->create_temporary_storage(return_type);

        //calltype.named_parameters["RETURN"] = return_slot_type;
        args.named["RETURN"] = return_slot;

        return_value = return_slot;

        co_await gen_call_functanoid(instanciation.value(), args);
    }

    co_return return_value;
}

QUX_SUBCO_MEMBER_FUNC_DEF(co_vmir_expression_emitter, gen_call_functanoid, void, (instanciation_reference what, vmir2::invocation_args args))
{
    auto const& call_args_types = what.parameters;

    // TODO: Support defaulted parameters.


    vm_invocation_args invocation_args;

    auto create_arg_value = [&](storage_index arg_expr_val, type_symbol arg_target_type) -> rpnx::general_coroutine< compiler, storage_index >
    {
        // TODO: Support PRValue args
        auto arg_expr_type = co_await inter->index_type(arg_expr_val);

        assert(is_ref(arg_expr_type));

        if (!is_ref(arg_target_type))
        {
            auto index = co_await inter->create_temporary_storage(arg_target_type);
            auto arg_final_ctor_func = subdotentity_reference{arg_target_type, "CONSTRUCTOR"};
            auto arg_final_dtor_func = subdotentity_reference{arg_target_type, "DESTRUCTOR"};
            vmir2::invocation_args ctor_args = {.named = {{"THIS", index}}, .positional = {arg_expr_val}};
            // These both need to be references or the constructor will probably infinite loop.
            assert(is_ref(arg_expr_type) && is_ref(make_mref(arg_target_type)));

            // TODO: instead of directly calling the constructor, call a special conversion function perhaps?
            co_await gen_call_functum(arg_final_ctor_func, ctor_args);

            // When we complete the constructor of the argument, we need to queue the destructor of the argument.
            // We also need to save the deferral so we can remove it from the deferral list prior to invoking
            // the target procedure (since ownership of the argument is transferred to the target procedure).
            co_return index;
        }
        else
        {
            throw rpnx::unimplemented();
        }
    };

    for (auto const& [name, arg_accepted_type] : call_args_types.named_parameters)
    {

        auto arg_expr = args.named.at(name);
        auto arg_expr_type = co_await inter->index_type(arg_expr);

        auto arg_index = co_await create_arg_value(arg_expr, arg_accepted_type);

        invocation_args.named[name] = arg_index;
    }

    throw rpnx::unimplemented();
}

QUX_SUBCO_MEMBER_FUNC_DEF(co_vmir_expression_emitter, generate, quxlang::vmir2::storage_index, (expression_symbol_reference expr))
{
    co_return (co_await inter->lookup_symbol(expr.symbol)).value();
}

QUX_SUBCO_MEMBER_FUNC_DEF(co_vmir_expression_emitter, generate, quxlang::vmir2::storage_index, (expression_binary input))
{
    auto lhs = co_await generate_expr(input.lhs);
    auto rhs = co_await generate_expr(input.rhs);

    type_symbol lhs_type = co_await inter->index_type(lhs);
    type_symbol rhs_type = co_await inter->index_type(rhs);

    type_symbol lhs_underlying_type = remove_ref(lhs_type);
    type_symbol rhs_underlying_type = remove_ref(rhs_type);

    type_symbol lhs_function = subdotentity_reference{lhs_underlying_type, "OPERATOR" + input.operator_str};
    type_symbol rhs_function = subdotentity_reference{rhs_underlying_type, "OPERATOR" + input.operator_str + "RHS"};
    call_type lhs_param_info{.named_parameters = {{"THIS", lhs_type}}, .positional_parameters = {rhs_type}};
    call_type rhs_param_info{.named_parameters = {{"THIS", rhs_type}}, .positional_parameters = {lhs_type}};

    auto lhs_exists_and_callable_with = co_await *c->lk_functum_exists_and_is_callable_with({.callee = lhs_function, .parameters = lhs_param_info});

    if (lhs_exists_and_callable_with)
    {
        auto lhs_args = vmir2::invocation_args{.named = {{"THIS", lhs}}, .positional = {rhs}};
        co_return co_await gen_call_functum(lhs_function, lhs_args);
    }

    auto rhs_exists_and_callable_with = co_await *c->lk_functum_exists_and_is_callable_with({.callee = rhs_function, .parameters = rhs_param_info});

    if (rhs_exists_and_callable_with)
    {
        auto rhs_args = vmir2::invocation_args{.named = {{"THIS", rhs}}, .positional = {lhs}};
        co_return co_await gen_call_functum(rhs_function, rhs_args);
    }

    throw std::logic_error("Found neither " + to_string(lhs_function) + " callable with (" + to_string(lhs_type) + ", " + to_string(rhs_type) + ") nor " + to_string(rhs_function) + " callable with (" + to_string(rhs_type) + ", " + to_string(lhs_type) + ")");
}

QUX_SUBCO_MEMBER_FUNC_DEF(co_vmir_expression_emitter, generate, quxlang::vmir2::storage_index, (expression_numeric_literal input))
{
    co_return co_await inter->create_numeric_literal(input.value);
}
