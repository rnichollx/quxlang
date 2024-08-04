#ifndef RPNX_QUXLANG_CO_VMIR_EXPRESSION_EMITTER_HEADER
#define RPNX_QUXLANG_CO_VMIR_EXPRESSION_EMITTER_HEADER

#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/class_layout.hpp"
#include "quxlang/data/expression_call.hpp"
#include "quxlang/data/vm_executable_unit.hpp"
#include "quxlang/data/vm_expression.hpp"
#include "quxlang/vmir2/vmir2.hpp"
#include "rpnx/simple_coroutine.hpp"
#include <quxlang/macros.hpp>
namespace quxlang
{

    template < typename CoroutineProvider >
    class co_vmir_expression_emitter
    {
        using storage_index = std::size_t;
        using deferral_index = std::size_t;

      private:
        CoroutineProvider prv;

      public:
        co_vmir_expression_emitter(CoroutineProvider prv) : prv(prv)
        {
        }

        auto typeof_vm_value(expression expr) -> typename CoroutineProvider::template co_type< quxlang::type_symbol >
        {
            rpnx::unimplemented();
            return {};
        }

        auto generate_expr(expression expr) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            if (typeis< expression_symbol_reference >(expr))
            {
                co_return co_await (this->generate(as< expression_symbol_reference >(std::move(expr))));
            }
            else if (typeis< expression_binary >(expr))
            {
                co_return co_await (this->generate(as< expression_binary >(std::move(expr))));
            }
            else if (typeis< expression_call >(expr))
            {
                co_return co_await (this->gen_call_expr(as< expression_call >(std::move(expr))));
            }
            else if (typeis< expression_numeric_literal >(expr))
            {
                co_return co_await this->generate(as< expression_numeric_literal >(std::move(expr)));
            }
            else if (typeis< expression_thisdot_reference >(expr))
            {
                co_return co_await this->generate(as< expression_thisdot_reference >(std::move(expr)));
            }
            else if (typeis< expression_this_reference >(expr))
            {
                co_return co_await this->generate(as< expression_this_reference >(std::move(expr)));
            }
            else if (typeis< expression_dotreference >(expr))
            {
                co_return co_await this->generate(as< expression_dotreference >(std::move(expr)));
            }
            else
            {
                throw std::logic_error("Unimplemented handler for " + std::string(expr.type().name()));
            }

            assert(false);
        }

      private:
        auto gen_call_expr(expression_call call) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto callee = co_await generate_expr(call.callee);

            type_symbol callee_type = co_await prv.index_type(callee);

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
                    call_t.named_parameters[*arg.name] = co_await prv.index_type(arg_val_idx);
                }
                else
                {
                    args.positional.push_back(arg_val_idx);
                    call_t.positional_parameters.push_back(co_await prv.index_type(arg_val_idx));
                }
            }

            if (!typeis< void_type >(callee_type_value.object_type))
            {
                call_t.named_parameters["THIS"] = callee_type;
                args.named["THIS"] = callee;
            }

            co_return co_await gen_call_functum(callee_type_value.function_type, args);
        }

        auto gen_call_functum(type_symbol func, vmir2::invocation_args args) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            std::cout << "gen_call_functum(" << quxlang::to_string(func) << ")" << quxlang::to_string(args)  << std::endl;

            call_type calltype;
            for (auto& arg : args.positional)
            {
                auto arg_type = co_await prv.index_type(arg);
                bool is_alive = co_await prv.slot_alive(arg);
                if (!is_alive)
                {
                    assert(!typeis< nvalue_slot >(arg_type));
                    arg_type = nvalue_slot{arg_type};
                }
                calltype.positional_parameters.push_back(arg_type);
            }
            for (auto& [name, arg] : args.named)
            {
                auto arg_type = co_await prv.index_type(arg);
                bool is_alive = co_await prv.slot_alive(arg);
                if (!is_alive)
                {
                    assert(!typeis< nvalue_slot >(arg_type));
                    arg_type = nvalue_slot{arg_type};
                }
                calltype.named_parameters[name] = arg_type;
            }

            instanciation_reference functanoid_unnormalized{.callee = func, .parameters = calltype};

            std::cout << "gen_call_functum B(" << quxlang::to_string(functanoid_unnormalized) << ")" << quxlang::to_string(args)  << std::endl;
            // Get call type
            auto instanciation = co_await prv.instanciation(functanoid_unnormalized);

            if (!instanciation)
            {
                throw std::logic_error("Cannot call " + to_string(func) + " with " + quxlang::to_string(calltype));
            }



            co_return co_await this->gen_call_functanoid(instanciation.value(), args);

        }

        auto gen_call_functanoid(instanciation_reference what, vmir2::invocation_args expression_args) ->  typename CoroutineProvider::template co_type< vmir2::storage_index >
        {

            std::cout << "gen_call_functanoid(" << quxlang::to_string(what) << ")" << quxlang::to_string(expression_args) << std::endl;
            auto const& call_args_types = what.parameters;

            // TODO: Support defaulted parameters.

            vmir2::invocation_args invocation_args;

            auto create_arg_value = [&](storage_index arg_expr_index, type_symbol arg_target_type) -> typename CoroutineProvider::template co_type< storage_index >
            {
                // TODO: Support PRValue args
                auto arg_expr_type = co_await prv.index_type(arg_expr_index);
                bool arg_alive = co_await prv.slot_alive(arg_expr_index);

                if (!arg_alive)
                {
                    assert(!is_ref(arg_expr_type));
                    arg_expr_type = nvalue_slot{arg_expr_type};
                }


                if (is_ref(arg_expr_type) && !is_ref(arg_target_type))
                {
                    std::cout << "gen_call_functanoid A(" << quxlang::to_string(what) << ")" << quxlang::to_string(arg_expr_type) << "->" << quxlang::to_string(arg_target_type)  << quxlang::to_string(expression_args) << std::endl;
                    auto index = co_await prv.create_temporary_storage(arg_target_type);
                    std::cout << "Created argument slot " << index << std::endl;
                    // Alive is false
                    auto arg_final_ctor_func = subdotentity_reference{arg_target_type, "CONSTRUCTOR"};

                    vmir2::invocation_args ctor_args = {.named = {{"THIS", index}}, .positional = {arg_expr_index}};
                    // These both need to be references or the constructor will probably infinite loop.
                    assert(is_ref(arg_expr_type) && is_ref(make_mref(arg_target_type)));

                    // TODO: instead of directly calling the constructor, call a special conversion function perhaps?
                    co_await gen_call_functum(arg_final_ctor_func, ctor_args);

                    // When we complete the constructor of the argument, we need to queue the destructor of the argument.
                    // We also need to save the deferral so we can remove it from the deferral list prior to invoking
                    // the target procedure (since ownership of the argument is transferred to the target procedure).
                    co_return index;
                }
                else if (!is_ref(arg_expr_type) && !is_ref(arg_target_type))
                {
                    std::cout << "gen_call_functanoid B(" << quxlang::to_string(what) << ")" << quxlang::to_string(arg_expr_type) << "->" << quxlang::to_string(arg_target_type) << std::endl;
                    if (arg_expr_type == arg_target_type)
                    {
                        co_return arg_expr_index;
                    }

                    auto index = co_await prv.create_temporary_storage(arg_target_type);
                        std::cout << "Created argument slot " << index << std::endl;
                    // Alive is false
                    auto arg_final_ctor_func = subdotentity_reference{arg_target_type, "CONSTRUCTOR"};

                    vmir2::invocation_args ctor_args = {.named = {{"THIS", index}}, .positional = {arg_expr_index}};

                    // TODO: instead of directly calling the constructor, call a special conversion function perhaps?
                    co_await gen_call_functum(arg_final_ctor_func, ctor_args);

                    co_return index;
                }
                else if (is_ref(arg_target_type))
                {
                    std::cout << "gen_call_functanoid C(" << quxlang::to_string(what) << ")" << quxlang::to_string(arg_expr_type) << "->" << quxlang::to_string(arg_target_type) << std::endl;
                    if (arg_expr_type == arg_target_type)
                    {
                        co_return arg_expr_index;
                    }

                    assert(is_ref_implicitly_convertible_by_syntax(arg_expr_type, arg_target_type));
                    // We need to hook into the provider because we might encounter a situation where
                    //  we need knowledge of base/derived classes etc. to do a cast.

                    co_return co_await prv.implicit_cast_reference(arg_expr_index, arg_target_type);
                }
                else
                {
                    throw rpnx::unimplemented();
                }
            };

            for (auto const& [name, arg_accepted_type] : call_args_types.named_parameters)
            {

                auto arg_expr_index = expression_args.named.at(name);

                auto arg_index = co_await create_arg_value(arg_expr_index, arg_accepted_type);

                invocation_args.named[name] = arg_index;
            }

            for (std::size_t i = 0; i < call_args_types.positional_parameters.size(); i++)
            {
                auto arg_accepted_type = call_args_types.positional_parameters.at(i);

                auto arg_expr_index = expression_args.positional.at(i);

                auto arg_index = co_await create_arg_value(arg_expr_index, arg_accepted_type);
                invocation_args.positional.push_back(arg_index);
            }

            auto return_type = co_await prv.functanoid_return_type(what);


            // Index 0 is defined to be the special "void" value.
            vmir2::storage_index retval = 0;

            if (!typeis< void_type >(return_type))
            {
                auto return_slot = co_await prv.create_temporary_storage(return_type);
                std::cout << "Created return slot " << return_slot << std::endl;

                // calltype.named_parameters["RETURN"] = return_slot_type;
                invocation_args.named["RETURN"] = return_slot;

                retval = return_slot;
            }



            //  assert(what.parameters.size() == args.size());

            if (invocation_args.named.contains("RETURN"))
            {
                assert(invocation_args.size() == what.parameters.size() + 1);
            }
            else
            {
                assert(invocation_args.size() == what.parameters.size());
            }

            co_await gen_invoke(what, invocation_args);


            co_return retval;
        }

        auto gen_invoke(instanciation_reference what, vmir2::invocation_args args) -> typename CoroutineProvider::template co_type< void >
        {
            if (args.named.contains("RETURN"))
            {
                assert(args.size() == what.parameters.size() + 1);
            }
            else
            {
                assert(args.size() == what.parameters.size());
            }
            std::string what_invoke = to_string(what);
            co_await prv.emit_invoke(what, args);
            co_return;
        }

        auto generate(expression_symbol_reference expr) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            co_return (co_await prv.lookup_symbol(expr.symbol)).value();
        }

        auto generate(expression_this_reference expr) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            throw rpnx::unimplemented();
            co_return 0;
        }

        auto generate(expression_call expr) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            throw rpnx::unimplemented();
            co_return 0;
        }

        auto generate(expression_binary input) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto lhs = co_await generate_expr(input.lhs);
            auto rhs = co_await generate_expr(input.rhs);

            type_symbol lhs_type = co_await prv.index_type(lhs);
            type_symbol rhs_type = co_await prv.index_type(rhs);

            type_symbol lhs_underlying_type = remove_ref(lhs_type);
            type_symbol rhs_underlying_type = remove_ref(rhs_type);

            type_symbol lhs_function = subdotentity_reference{lhs_underlying_type, "OPERATOR" + input.operator_str};
            type_symbol rhs_function = subdotentity_reference{rhs_underlying_type, "OPERATOR" + input.operator_str + "RHS"};
            call_type lhs_param_info{.named_parameters = {{"THIS", lhs_type}}, .positional_parameters = {rhs_type}};
            call_type rhs_param_info{.named_parameters = {{"THIS", rhs_type}}, .positional_parameters = {lhs_type}};

            auto lhs_exists_and_callable_with = co_await prv.instanciation({.callee = lhs_function, .parameters = lhs_param_info});

            if (lhs_exists_and_callable_with)
            {
                auto lhs_args = vmir2::invocation_args{.named = {{"THIS", lhs}}, .positional = {rhs}};
                co_return co_await gen_call_functum(lhs_function, lhs_args);
            }

            auto rhs_exists_and_callable_with = co_await prv.instanciation({.callee = rhs_function, .parameters = rhs_param_info});

            if (rhs_exists_and_callable_with)
            {
                auto rhs_args = vmir2::invocation_args{.named = {{"THIS", rhs}}, .positional = {lhs}};
                co_return co_await gen_call_functum(rhs_function, rhs_args);
            }

            throw std::logic_error("Found neither " + to_string(lhs_function) + " callable with (" + to_string(lhs_type) + ", " + to_string(rhs_type) + ") nor " + to_string(rhs_function) + " callable with (" + to_string(rhs_type) + ", " + to_string(lhs_type) + ")");
        }

        auto generate(expression_numeric_literal input) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            co_return co_await prv.create_numeric_literal(input.value);
        }

        auto generate(expression_thisdot_reference what) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto this_reference = subdotentity_reference{.parent = context_reference{}, .subdotentity_name = "THIS"};
            auto value = co_await prv.lookup_symbol(this_reference);
            if (!value)
            {
                throw std::logic_error("Cannot find " + to_string(this_reference));
            }
            co_return value.value();
        }

        auto generate(expression_dotreference what) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto parent = co_await generate_expr(what.lhs);

            co_return co_await generate_field_access(parent, what.field_name);
        }

        auto generate_field_access(storage_index base, std::string field_name) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto base_type = co_await prv.index_type(base);
            auto base_type_noref = quxlang::remove_ref(base_type);

            class_layout layout = co_await prv.class_layout(base_type_noref);

            for (class_field_info const& field : layout.fields)
            {
                if (field.name == field_name)
                {
                    vmir2::access_field access;
                    access.base_index = base;
                    access.offset = field.offset;
                    type_symbol result_ref_type = recast_reference(base_type, field.type);
                    access.store_index = co_await prv.create_temporary_storage(result_ref_type);
                    std::cout << "Created field access " << access.store_index << " for " << field_name << " in " << to_string(base_type) << std::endl;

                    co_await prv.emit_instruction(access);
                    co_return access.store_index;
                }
            }

            throw std::logic_error("Cannot find field " + field_name + " in " + to_string(base_type));
        }
    };
} // namespace quxlang

#endif // RPNX_QUXLANG_CO_VMIR_EXPRESSION_EMITTER_HEADER