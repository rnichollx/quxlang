#ifndef RPNX_QUXLANG_CO_VMIR_EXPRESSION_EMITTER_HEADER
#define RPNX_QUXLANG_CO_VMIR_EXPRESSION_EMITTER_HEADER

#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/class_layout.hpp"
#include "quxlang/data/expression_call.hpp"
#include "quxlang/data/vm_executable_unit.hpp"
#include "quxlang/data/vm_expression.hpp"
#include "quxlang/res/implicitly_convertible_to.hpp"
#include "quxlang/res/symbol_type.hpp"
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
                std::cout << "generate binary" << as<expression_binary>(expr).operator_str << std::endl;
                co_return co_await (this->generate(as< expression_binary >(std::move(expr))));
            }
            else if (typeis< expression_call >(expr))
            {
                std::cout << "generate call" << std::endl;// << as<expression_call>(expr).callee << std::endl;

                co_return co_await (this->gen_call_expr(as< expression_call >(std::move(expr))));
            }
            else if (typeis< expression_numeric_literal >(expr))
            {
                std::cout << "Generate a numeric literal" << std::endl;
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
        auto gen_idx_conversion(storage_index idx, type_symbol to_type) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            vmir2::invocation_args args;
            args.named["OTHER"] = idx;
            co_return co_await gen_call_ctor(to_type, std::move(args));
        }

        auto gen_call_ctor(type_symbol new_type, vmir2::invocation_args args) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto ctor = subdotentity_reference{.parent = new_type , .subdotentity_name = "CONSTRUCTOR"};
            auto new_object = co_await prv.create_temporary_storage(new_type);
            args.named["THIS"] = new_object;
            auto retval = co_await gen_call_functum(ctor, args);
            assert(retval == 0);
            co_return new_object;
        }


        auto gen_call_expr(expression_call call) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            std::cout << "gen_call_expr A()" << std::endl;
            auto callee = co_await generate_expr(call.callee);


            type_symbol callee_type = co_await prv.index_type(callee);
            std::cout << "gen_call_expr B() -> callee_type=" << quxlang::to_string(callee_type) << std::endl;

            std::string callee_type_string = to_string(callee_type);

            if (!typeis< bound_type_reference >(callee_type))
            {
                auto value_type = remove_ref(callee_type);
                auto operator_call = subdotentity_reference{.parent = value_type, .subdotentity_name = "OPERATOR()"};
                callee = co_await prv.create_binding(callee, operator_call);
                callee_type = co_await prv.index_type(callee);
            }


            type_symbol bound_symbol = as< bound_type_reference >(callee_type).bound_symbol;

            type_symbol carried_type = as< bound_type_reference >(callee_type).carried_type;
            symbol_kind bound_symbol_kind = co_await prv.symbol_type(bound_symbol);

            vmir2::invocation_args args;
            std::string callee_type_string2 = to_string(as< bound_type_reference >(callee_type));

            std::cout << "requesting generate call to bindval=" << to_string(carried_type) << " bindsym=" << to_string(bound_symbol) << std::endl;




            std::string callee_type_string3 = to_string(callee_type);


            for (auto& arg : call.args)
            {
                auto arg_val_idx = co_await generate_expr(arg.value);

                // TODO: we shouldn't drop bindings here, since there might be a reason to accept
                // binding values later.
                // Instead all bindings should be implicitly convertible to their bound value.
                arg_val_idx= co_await prv.index_binding(arg_val_idx);

                if (arg.name)
                {
                    args.named[*arg.name] = arg_val_idx;
                }
                else
                {
                    args.positional.push_back(arg_val_idx);
                }
            }

            if (!typeis< void_type >(as< bound_type_reference >(callee_type).carried_type))
            {
                auto bound_index = co_await prv.index_binding(callee);
                args.named["THIS"] = bound_index;
            }

            if (bound_symbol_kind == symbol_kind::user_class || bound_symbol_kind == symbol_kind::builtin_class)
            {
                if (!typeis< void_type >(as< bound_type_reference >(callee_type).carried_type))
                {
                    throw std::logic_error("this is bug...");
                }
                auto object_type = as< bound_type_reference >(callee_type).bound_symbol;

                co_return co_await gen_call_ctor(object_type, args);

            }

            co_return co_await gen_call_functum(as< bound_type_reference >(callee_type).bound_symbol, args);
        }

        auto gen_call_functum(type_symbol func, vmir2::invocation_args args) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            std::cout << "gen_call_functum(" << quxlang::to_string(func) << ")" << quxlang::to_string(args) << std::endl;

            call_type calltype;
            for (auto& arg : args.positional)
            {
                auto arg_type = co_await prv.index_type(arg);
                bool is_alive = co_await prv.slot_alive(arg);
                if (!is_alive)
                {
                    assert(typeis< nvalue_slot >(arg_type));
                    //arg_type = nvalue_slot{arg_type};
                }
                calltype.positional_parameters.push_back(arg_type);
            }
            for (auto& [name, arg] : args.named)
            {
                auto arg_type = co_await prv.index_type(arg);
                bool is_alive = co_await prv.slot_alive(arg);

                std::cout << " arg name=" << name << " index=" << arg << " is_alive=" << is_alive << std::endl;
                if (!is_alive)
                {
                    assert(typeis< nvalue_slot >(arg_type));
                }
                calltype.named_parameters[name] = arg_type;
            }

            instanciation_reference functanoid_unnormalized{.callee = func, .parameters = calltype};

            std::cout << "gen_call_functum B(" << quxlang::to_string(functanoid_unnormalized) << ")" << quxlang::to_string(args) << std::endl;
            // Get call type
            auto instanciation = co_await prv.instanciation(functanoid_unnormalized);

            if (!instanciation)
            {
                throw std::logic_error("Cannot call " + to_string(func) + " with " + quxlang::to_string(calltype));
            }

            co_return co_await this->gen_call_functanoid(instanciation.value(), args);
        }

        auto gen_call_functanoid(instanciation_reference what, vmir2::invocation_args expression_args) -> typename CoroutineProvider::template co_type< vmir2::storage_index >
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
                   // arg_expr_type = nvalue_slot{arg_expr_type};
                }

                if (is_ref(arg_expr_type) && !is_ref(arg_target_type))
                {
                    std::cout << "gen_call_functanoid A(" << quxlang::to_string(what) << ")" << quxlang::to_string(arg_expr_type) << "->" << quxlang::to_string(arg_target_type) << quxlang::to_string(expression_args) << std::endl;
                    auto index = co_await prv.create_temporary_storage(arg_target_type);
                    std::cout << "Created argument slot " << index << std::endl;
                    // Alive is false
                    auto arg_final_ctor_func = subdotentity_reference{arg_target_type, "CONSTRUCTOR"};

                    vmir2::invocation_args ctor_args = {.named = {{"THIS", index},  {"OTHER", arg_expr_index}}};
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

                    vmir2::invocation_args ctor_args = {.named = {{"THIS", index}, {"OTHER", arg_expr_index} } };

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

                    auto query = quxlang::implicitly_convertible_to_query();

                    query.from = arg_expr_type;
                    query.to = arg_target_type;

                    bool convertible = co_await prv.implicitly_convertible_to(query.from, query.to);

                    if (!convertible)
                    {
                        throw std::runtime_error("Cannot convert " + to_string(arg_expr_type) + " to " + to_string(arg_target_type));
                    }
                    // assert(is_ref_implicitly_convertible_by_syntax(arg_expr_type, arg_target_type));
                    //  We need to hook into the provider because we might encounter a situation where
                    //   we need knowledge of base/derived classes etc. to do a cast.
                    auto new_index = co_await gen_implicit_conversion(arg_expr_index, arg_target_type);

                    co_return new_index;
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

        auto gen_reinterpret_reference(storage_index ref_index, type_symbol target_ref_type) -> typename CoroutineProvider::template co_type< storage_index >
        {
            auto ref_type = co_await prv.index_type(ref_index);

            if (!is_ref(target_ref_type) || !is_ref(ref_type))
            {
                throw std::logic_error("Cannot gen_reinterpret_reference reinterpret non-reference types");
            }

            auto new_index = co_await prv.create_temporary_storage(target_ref_type);

            vmir2::access_field make_ref;
            make_ref.base_index = ref_index;
            make_ref.store_index = new_index;
            make_ref.offset = 0;

            co_await prv.emit(make_ref);

            co_return new_index;
        }

        auto gen_reference_conversion(storage_index value_index, type_symbol target_reference_type) -> typename CoroutineProvider::template co_type< storage_index >
        {
            // TODO: Support dynamic/static casts
            co_return co_await gen_reinterpret_reference(value_index, target_reference_type);
        }

        auto gen_value_conversion(storage_index value_index, type_symbol target_value_type) -> typename CoroutineProvider::template co_type< storage_index >
        {
            // TODO: support conversion other than via constructor.
            co_return co_await gen_value_constructor_conversion(value_index, target_value_type);
        }

        auto gen_value_constructor_conversion(storage_index value_index, type_symbol target_value_type) -> typename CoroutineProvider::template co_type< storage_index >
        {

            type_symbol value_type = co_await prv.index_type(value_index);
            std::cout << "gen_value_conversion(" << value_index << "(" << to_string(value_type) << "), " << to_string(target_value_type) << ")" << std::endl;

            auto new_value_index = co_await prv.create_temporary_storage(target_value_type);

            auto conversion_functum = subdotentity_reference{target_value_type, "CONSTRUCTOR"};

            vmir2::invocation_args args = {.named = {{"THIS", new_value_index}, {"OTHER", value_index}}};

            co_return co_await gen_call_functum(conversion_functum, args);
        }

        auto gen_implicit_conversion(storage_index value_index, type_symbol target_type) -> typename CoroutineProvider::template co_type< storage_index >
        {
            type_symbol value_type = co_await prv.index_type(value_index);
            std::cout << "gen_implicit_conversion(" << value_index << "(" << to_string(value_type) << "), " << to_string(target_type) << ")" << std::endl;

            if (value_type == target_type)
            {
                co_return value_index;
            }

            if (is_ref(target_type))
            {
                co_return co_await gen_reference_conversion(value_index, target_type);
            }
            else
            {
                co_return co_await gen_value_conversion(value_index, target_type);
            }
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

            vmir2::invoke ivk;
            ivk.what = what;
            ivk.args = args;

            co_await prv.emit(ivk);
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
            call_type lhs_param_info{.named_parameters = {{"THIS", lhs_type}, {"OTHER", rhs_type}}};
            call_type rhs_param_info{.named_parameters = {{"THIS", rhs_type}, {"OTHER", lhs_type}}};

            auto lhs_exists_and_callable_with = co_await prv.instanciation({.callee = lhs_function, .parameters = lhs_param_info});

            if (lhs_exists_and_callable_with)
            {
                auto lhs_args = vmir2::invocation_args{.named = {{"THIS", lhs}, {"OTHER", rhs}}};
                co_return co_await gen_call_functum(lhs_function, lhs_args);
            }

            auto rhs_exists_and_callable_with = co_await prv.instanciation({.callee = rhs_function, .parameters = rhs_param_info});

            if (rhs_exists_and_callable_with)
            {
                auto rhs_args = vmir2::invocation_args{.named = {{"THIS", rhs}, {"OTHER", lhs}}};
                co_return co_await gen_call_functum(rhs_function, rhs_args);
            }

            throw std::logic_error("Found neither " + to_string(lhs_function) + " callable with (" + to_string(lhs_type) + ", " + to_string(rhs_type) + ") nor " + to_string(rhs_function) + " callable with (" + to_string(rhs_type) + ", " + to_string(lhs_type) + ")");
        }

        auto generate(expression_numeric_literal input) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto val = co_await prv.create_numeric_literal(input.value);
            assert(val != 0);
            auto val_type = co_await prv.index_type(val);
            std::cout << "Generated numeric literal " << val << " of type " << to_string(val_type) << std::endl;
                co_return val;
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

                    co_await prv.emit(access);
                    co_return access.store_index;
                }
            }

            throw std::logic_error("Cannot find field " + field_name + " in " + to_string(base_type));
        }
    };
} // namespace quxlang

#endif // RPNX_QUXLANG_CO_VMIR_EXPRESSION_EMITTER_HEADER