// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_RES_EXPR_CO_VMIR_EXPRESSION_EMITTER_HEADER_GUARD
#define QUXLANG_RES_EXPR_CO_VMIR_EXPRESSION_EMITTER_HEADER_GUARD

#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/class_layout.hpp"
#include "quxlang/data/contextual_type_reference.hpp"
#include "quxlang/data/expression_call.hpp"
#include "quxlang/data/vm_executable_unit.hpp"
#include "quxlang/data/vm_expression.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/res/implicitly_convertible_to.hpp"
#include "quxlang/res/symbol_type.hpp"
#include "quxlang/vmir2/vmir2.hpp"
#include "rpnx/simple_coroutine.hpp"
#include <quxlang/macros.hpp>

namespace quxlang
{

    template < typename T >
    concept expr_co_provider = requires(T t) {
        {
            std::declval< typename T::template co_type< quxlang::type_symbol > >()
        };
        {
            std::declval< typename T::template co_type< quxlang::vmir2::storage_index > >()
        };
    };

    template < expr_co_provider CoroutineProvider >
    class co_vmir_expression_emitter
    {
        using storage_index = std::size_t;
        using deferral_index = std::size_t;

      private:
        CoroutineProvider prv;
        vmir2::executable_block_generation_state& exec;
        type_symbol ctx;

      public:
        co_vmir_expression_emitter(CoroutineProvider prv, type_symbol ctx, vmir2::executable_block_generation_state& exec) : prv(prv), ctx(ctx), exec(exec)
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
                std::cout << "generate binary" << as< expression_binary >(expr).operator_str << std::endl;
                co_return co_await (this->generate(as< expression_binary >(std::move(expr))));
            }
            else if (typeis< expression_call >(expr))
            {
                std::cout << "generate call" << std::endl; // << as<expression_call>(expr).callee << std::endl;

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

        auto gen_call_functum(type_symbol func, vmir2::invocation_args args) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            std::cout << "gen_call_functum(" << quxlang::to_string(func) << ")" << quxlang::to_string(args) << std::endl;

            calltype calltype;
            for (auto& arg : args.positional)
            {
                auto arg_type = this->current_type(arg);
                bool is_alive = this->slot_alive(arg);
                if (!is_alive)
                {
                    assert(typeis< nvalue_slot >(arg_type));
                    // arg_type = nvalue_slot{arg_type};
                }
                calltype.positional.push_back(arg_type);
            }
            for (auto& [name, arg] : args.named)
            {
                auto arg_type = current_type(arg);
                bool is_alive = slot_alive(arg);

                std::cout << " arg name=" << name << " index=" << arg << " is_alive=" << is_alive << std::endl;
                if (!is_alive)
                {
                    assert(typeis< nvalue_slot >(arg_type));
                }
                calltype.named[name] = arg_type;
            }

            instantiation_type functanoid_unnormalized{.callee = func, .parameters = calltype};

            std::cout << "gen_call_functum B(" << quxlang::to_string(functanoid_unnormalized) << ")" << quxlang::to_string(args) << std::endl;
            // Get call type
            auto instanciation = co_await prv.instanciation(functanoid_unnormalized);

            if (!instanciation)
            {
                throw std::logic_error("Cannot call " + to_string(func) + " with " + quxlang::to_string(calltype));
            }

            co_return co_await this->gen_call_functanoid(instanciation.value(), args);
        }

      private:
        auto create_temporary_storage(type_symbol type) -> vmir2::storage_index
        {
            return exec.create_temporary(MOVEREL(type));
        }

        auto current_type(storage_index idx) -> type_symbol
        {
            return exec.current_type(idx);
        }

        auto index_binding(storage_index idx)
        {
            return exec.index_binding(idx);
        }

        auto create_binding(storage_index bindval, type_symbol bind_type)
        {
            return exec.create_binding(bindval, bind_type);
        }

        auto emit(auto val)
        {
            return exec.emit(val);
        }

        auto slot_alive(storage_index slot)
        {
            return exec.slot_alive(slot);
        }

        auto create_reference_internal(vmir2::storage_index index, type_symbol const& new_type)
        {
            // This function is used to handle the case where we have an index and need to force it into a
            // reference type.
            // This is mainly used in three places, implied ctor "THIS" argument, dtor, and when a symbol
            // is encountered during an expression.
            auto ty = this->current_type(index);

            vmir2::storage_index temp = create_temporary_storage(new_type);

            vmir2::make_reference ref;
            ref.value_index = index;
            ref.reference_index = temp;

            // TODO: make this not a hack
            this->emit(ref);

            std::string slot_type = quxlang::to_string(exec.current_type(index));
            // this is kind of a hack, but we can presume this after the make_reference call.

            exec.current_slot_states[temp].alive = true;

            return temp;
        }

        auto lookup_symbol(type_symbol sym) -> typename CoroutineProvider::template co_type< std::optional< vmir2::storage_index > >
        {
            std::string symbol_str = to_string(sym);

            bool a = typeis< subsymbol >(sym);
            bool b;
            if (typeis<subsymbol>(sym))
            {
                b = typeis< context_reference >(as< subsymbol >(sym).of);
            }
            else
            {
                b = false;
            }

            if (typeis< subsymbol >(sym) && typeis< context_reference >(as< subsymbol >(sym).of))
            {
                std::string const& name = as< subsymbol >(sym).name;
                std::cout << "lookup " << name << std::endl;
                auto lookup = this->exec.local_lookup(name);
                if (lookup)
                {
                    auto lookup_type = this->current_type(lookup.value());
                    std::cout << "lookup " << name << " -> " << lookup.value() << " type=" << to_string(lookup_type) << std::endl;

                    if (!is_ref(lookup_type))
                    {
                        lookup = create_reference_internal(*lookup, make_mref(lookup_type));
                    }

                    co_return lookup;
                }
            }
            auto canonical_symbol_opt = co_await prv.lookup(contextual_type_reference{.context = ctx, .type = sym});

            if (!canonical_symbol_opt)
            {
                co_return std::nullopt;
            }

            auto canonical_symbol = canonical_symbol_opt.value();

            auto kind = co_await prv.symbol_type(canonical_symbol);

            vmir2::storage_index index = 0;

            auto binding = create_binding(0, canonical_symbol);

            if (kind == quxlang::symbol_kind::global_variable)
            {
                auto variable_type = co_await prv.variable_type(canonical_symbol);
                index = this->create_reference_internal(binding, variable_type);
            }
            else
            {
                index = binding;
            }

            type_symbol index_type = this->current_type(index);

            std::string index_type_str = to_string(index_type);

            co_return index;
        }

        auto gen_idx_conversion(storage_index idx, type_symbol to_type) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            vmir2::invocation_args args;
            args.named["OTHER"] = idx;
            co_return co_await gen_call_ctor(to_type, std::move(args));
        }

        auto gen_call_ctor(type_symbol new_type, vmir2::invocation_args args) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto ctor = submember{.of = new_type, .name = "CONSTRUCTOR"};
            auto new_object = create_temporary_storage(new_type);
            args.named["THIS"] = new_object;
            auto retval = co_await gen_call_functum(ctor, args);
            assert(retval == 0);
            co_return new_object;
        }

        auto gen_call_expr(expression_call call) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            std::cout << "gen_call_expr A()" << std::endl;
            auto callee = co_await generate_expr(call.callee);

            type_symbol callee_type = this->current_type(callee);
            std::cout << "gen_call_expr B() -> callee_type=" << quxlang::to_string(callee_type) << std::endl;

            std::string callee_type_string = to_string(callee_type);

            if (!typeis< bound_type_reference >(callee_type))
            {
                auto value_type = remove_ref(callee_type);
                auto operator_call = submember{.of = value_type, .name = "OPERATOR()"};
                callee = this->create_binding(callee, operator_call);
                callee_type = this->current_type(callee);
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
                arg_val_idx = this->index_binding(arg_val_idx);

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
                auto bound_index = this->index_binding(callee);
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

        auto create_numeric_literal(std::string str)
        {
            return exec.create_numeric_literal(str);
        }

        auto gen_call_functanoid(instantiation_type what, vmir2::invocation_args expression_args) -> typename CoroutineProvider::template co_type< vmir2::storage_index >
        {

            std::cout << "gen_call_functanoid(" << quxlang::to_string(what) << ")" << quxlang::to_string(expression_args) << std::endl;
            auto const& call_args_types = what.parameters;

            // TODO: Support defaulted parameters.

            vmir2::invocation_args invocation_args;

            auto create_arg_value = [&](storage_index arg_expr_index, type_symbol arg_target_type) -> typename CoroutineProvider::template co_type< storage_index >
            {
                // TODO: Support PRValue args
                auto arg_expr_type = this->current_type(arg_expr_index);
                bool arg_alive = this->slot_alive(arg_expr_index);

                if (!arg_alive)
                {
                    assert(!is_ref(arg_expr_type));
                    // arg_expr_type = nvalue_slot{arg_expr_type};
                }

                if (is_ref(arg_expr_type) && !is_ref(arg_target_type))
                {
                    std::cout << "gen_call_functanoid A(" << quxlang::to_string(what) << ")" << quxlang::to_string(arg_expr_type) << "->" << quxlang::to_string(arg_target_type) << quxlang::to_string(expression_args) << std::endl;
                    auto index = create_temporary_storage(arg_target_type);
                    std::cout << "Created argument slot " << index << std::endl;
                    // Alive is false
                    auto arg_final_ctor_func = submember{arg_target_type, "CONSTRUCTOR"};

                    vmir2::invocation_args ctor_args = {.named = {{"THIS", index}, {"OTHER", arg_expr_index}}};
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

                    auto index = create_temporary_storage(arg_target_type);
                    std::cout << "Created argument slot " << index << std::endl;
                    // Alive is false
                    auto arg_final_ctor_func = submember{arg_target_type, "CONSTRUCTOR"};

                    vmir2::invocation_args ctor_args = {.named = {{"THIS", index}, {"OTHER", arg_expr_index}}};

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

            for (auto const& [name, arg_accepted_type] : call_args_types.named)
            {

                auto arg_expr_index = expression_args.named.at(name);

                auto arg_index = co_await create_arg_value(arg_expr_index, arg_accepted_type);

                invocation_args.named[name] = arg_index;
            }

            for (std::size_t i = 0; i < call_args_types.positional.size(); i++)
            {
                auto arg_accepted_type = call_args_types.positional.at(i);

                auto arg_expr_index = expression_args.positional.at(i);

                auto arg_index = co_await create_arg_value(arg_expr_index, arg_accepted_type);
                invocation_args.positional.push_back(arg_index);
            }

            auto return_type = co_await prv.functanoid_return_type(what);

            // Index 0 is defined to be the special "void" value.
            vmir2::storage_index retval = 0;

            if (!typeis< void_type >(return_type))
            {
                auto return_slot = create_temporary_storage(return_type);
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
            auto ref_type = this->current_type(ref_index);

            if (!is_ref(target_ref_type) || !is_ref(ref_type))
            {
                throw std::logic_error("Cannot gen_reinterpret_reference reinterpret non-reference types");
            }

            auto new_index = create_temporary_storage(target_ref_type);

            vmir2::access_field make_ref;
            make_ref.base_index = ref_index;
            make_ref.store_index = new_index;
            make_ref.offset = 0;

            this->emit(make_ref);

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

            type_symbol value_type = this->current_type(value_index);
            std::cout << "gen_value_conversion(" << value_index << "(" << to_string(value_type) << "), " << to_string(target_value_type) << ")" << std::endl;

            auto new_value_index = create_temporary_storage(target_value_type);

            auto conversion_functum = submember{target_value_type, "CONSTRUCTOR"};

            vmir2::invocation_args args = {.named = {{"THIS", new_value_index}, {"OTHER", value_index}}};

            co_return co_await gen_call_functum(conversion_functum, args);
        }

        auto gen_implicit_conversion(storage_index value_index, type_symbol target_type) -> typename CoroutineProvider::template co_type< storage_index >
        {
            type_symbol value_type = this->current_type(value_index);
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

        auto gen_invoke_builtin(instantiation_type what, vmir2::invocation_args const &args) -> typename CoroutineProvider::template co_type< void >
        {
            auto callee = as< selection_reference >(what.callee);

            assert(callee.overload.builtin);

            auto functum = callee.templexoid;

            std::optional< type_symbol > class_type;
            bool member = false;
            std::string name;

            if (typeis< submember >(functum))
            {
                member = true;
                class_type = as< submember >(functum).of;
                name = as< submember >(functum).name;
            }
            else if (typeis< subsymbol >(functum))
            {
                member = false;
                class_type = as< subsymbol >(functum).of;
                name = as< subsymbol >(functum).name;
            }
            else
            {
                throw std::logic_error("Expected functum to be a subentity_reference or subdotentity_reference");
            }

            bool is_operator;
            bool is_rhs;

            std::string operator_value;

            if (name.starts_with("OPERATOR"))
            {
                is_operator = true;
                operator_value = name.substr(8);
                if (operator_value.ends_with("RHS"))
                {
                    is_rhs = true;
                    operator_value = operator_value.substr(0, operator_value.size() - 3);
                }
                else
                {
                    is_rhs = false;
                }
            }
            else
            {
                is_operator = false;
            }

            type_symbol lhs_type;
            type_symbol rhs_type;

            if (is_operator && is_rhs && args.named.contains("THIS") && args.named.contains("OTHER"))
            {
                auto rhs_index = args.named.at("THIS");
                rhs_type = this->current_type(rhs_index);
                auto lhs_index = args.named.at("OTHER");
                lhs_type = this->current_type(lhs_index);
            }
            else if (is_operator && args.named.contains("THIS") && args.named.contains("OTHER"))
            {
                auto rhs_index = args.named.at("OTHER");
                rhs_type = this->current_type(rhs_index);
                auto lhs_index = args.named.at("THIS");
                lhs_type = this->current_type(lhs_index);
            }

            if (assignment_operators.contains(operator_value))
            {

            }

            std::string what_str = to_string(what);
            std::string args_str = to_string(args);
            vmir2::invoke ivk;
            ivk.what = what;
            ivk.args = args;

            this->emit(ivk);

            co_return;


        }

        auto gen_invoke(instantiation_type what, vmir2::invocation_args args) -> typename CoroutineProvider::template co_type< void >
        {
            if (true && typeis< selection_reference >(what.callee) && as< selection_reference >(what.callee).overload.builtin)
            {
                co_return co_await gen_invoke_builtin(what, args);
            }

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

            this->emit(ivk);
            co_return;
        }

        auto generate(expression_symbol_reference expr) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {

            auto value_opt = (co_await this->lookup_symbol(expr.symbol));

            if (!value_opt.has_value())
            {
                throw std::logic_error("Expected symbol " + quxlang::to_string(expr.symbol) + " to be defined.");
            }

            co_return value_opt.value();
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

            type_symbol lhs_type = this->current_type(lhs);
            type_symbol rhs_type = this->current_type(rhs);

            type_symbol lhs_underlying_type = remove_ref(lhs_type);
            type_symbol rhs_underlying_type = remove_ref(rhs_type);

            type_symbol lhs_function = submember{lhs_underlying_type, "OPERATOR" + input.operator_str};
            type_symbol rhs_function = submember{rhs_underlying_type, "OPERATOR" + input.operator_str + "RHS"};
            calltype lhs_param_info{.named = {{"THIS", lhs_type}, {"OTHER", rhs_type}}};
            calltype rhs_param_info{.named = {{"THIS", rhs_type}, {"OTHER", lhs_type}}};

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
            auto val = this->create_numeric_literal(input.value);
            assert(val != 0);
            auto val_type = this->current_type(val);
            std::cout << "Generated numeric literal " << val << " of type " << to_string(val_type) << std::endl;
            co_return val;
        }

        auto generate(expression_thisdot_reference what) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto this_reference = submember{.of = context_reference{}, .name = "THIS"};
            auto value = co_await this->lookup_symbol(this_reference);
            if (!value)
            {
                throw std::logic_error("Cannot find " + to_string(this_reference));
            }
            co_return value.value();
        }

        auto generate(expression_dotreference what) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto parent = co_await generate_expr(what.lhs);

            co_return co_await generate_dot_access(parent, what.field_name);
        }

        auto generate_dot_access(storage_index base, std::string field_name) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {

            auto base_type = this->current_type(base);
            auto base_type_noref = quxlang::remove_ref(base_type);

            // First try to find a field with this name
            class_layout layout = co_await prv.class_layout(base_type_noref);

            for (class_field_info const& field : layout.fields)
            {
                if (field.name == field_name)
                {
                    vmir2::access_field access;
                    access.base_index = base;
                    access.offset = field.offset;
                    type_symbol result_ref_type = recast_reference(base_type, field.type);
                    access.store_index = create_temporary_storage(result_ref_type);
                    std::cout << "Created field access " << access.store_index << " for " << field_name << " in " << to_string(base_type) << std::endl;

                    this->emit(access);
                    co_return access.store_index;
                }
            }

            // If no field is found, look for a member function
            auto member_func = submember{.of = base_type_noref, .name = field_name};
            auto lookup_result = co_await prv.lookup(contextual_type_reference{.context = ctx, .type = member_func});

            if (lookup_result)
            {
                // Create a binding to the member function with the base object
                auto binding = create_binding(base, lookup_result.value());
                std::cout << "Created member function binding " << binding << " for " << field_name << " in " << to_string(base_type) << std::endl;
                co_return binding;
            }


            throw std::logic_error("Cannot find field " + field_name + " in " + to_string(base_type));
        }
    };

    template < typename Prv >
    auto emit_expression(Prv pr, type_symbol ctx, vmir2::executable_block_generation_state& exec, expression expr) -> typename Prv::template co_type< vmir2::storage_index >
    {
        co_vmir_expression_emitter expr_emit(pr, ctx, exec);
        co_return co_await expr_emit.generate_expr(expr);
    }
} // namespace quxlang

#endif // RPNX_QUXLANG_CO_VMIR_EXPRESSION_EMITTER_HEADER