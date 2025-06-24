// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_RES_EXPR_CODEGEN_VMIR_EXPRESSION_EMITTER_HEADER_GUARD
#define QUXLANG_RES_EXPR_CODEGEN_VMIR_EXPRESSION_EMITTER_HEADER_GUARD

#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/class_layout.hpp"
#include "quxlang/data/contextual_type_reference.hpp"
#include "quxlang/data/expression_call.hpp"
#include "quxlang/data/vm_executable_unit.hpp"
#include "quxlang/data/vm_expression.hpp"
#include "quxlang/exception.hpp"
#include "quxlang/intrinsic_classifier.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/res/implicitly_convertible_to.hpp"
#include "quxlang/res/symbol_type.hpp"
#include "quxlang/vmir2/assembly.hpp"
#include "quxlang/vmir2/state_engine.hpp"
#include "quxlang/vmir2/vmir2.hpp"
#include "rpnx/simple_coroutine.hpp"
#include <quxlang/macros.hpp>

namespace quxlang
{

    template < typename T >
    concept expr_co_provider = requires(T t) {
        { std::declval< typename T::template co_type< quxlang::type_symbol > >() };
        { std::declval< typename T::template co_type< quxlang::vmir2::storage_index > >() };
    };

    template < expr_co_provider CoroutineProvider >
    class co_vmir_generator
    {
        using storage_index = std::uint64_t;
        using deferral_index = std::uint64_t;
        using value_index = std::uint64_t;
        using block_index = std::uint64_t;

        struct codegen_literal;
        struct codegen_local;
        struct codegen_argument;
        struct codegen_block
        {
            std::map< storage_index, vmir2::slot_state > entry_state;
            std::map< storage_index, vmir2::slot_state > current_state;
            std::vector< vmir2::vm_instruction > instructions;
            std::optional< vmir2::vm_terminator > terminator;
            std::optional< std::string > dbg_name;

            RPNX_MEMBER_METADATA(codegen_block, entry_state, instructions, terminator, dbg_name);
        };

        struct codegen_argument
        {
            type_symbol type;
            std::optional< value_index > index;
        };

        struct codegen_binding
        {
            type_symbol bound_symbol;
            storage_index bound_value;
        };

        struct codegen_local
        {
            type_symbol type;
        };

        using codegen_value = rpnx::variant< codegen_binding, codegen_literal, codegen_local >;

        struct codegen_state
        {
            std::vector< codegen_value > genvalues{codegen_binding{.bound_symbol = void_type(), .bound_value = 0}};
            std::map< storage_index, codegen_block > blocks;
            std::vector< codegen_argument > positional_args;
            std::map< std::string, codegen_argument > named_args;
            std::map< type_symbol, type_symbol > non_trivial_dtors;
        };

      private:
        CoroutineProvider prv;
        codegen_state state;
        type_symbol ctx;

      public:
        co_vmir_generator(CoroutineProvider prv, type_symbol ctx) : prv(prv), ctx(ctx)
        {
        }

        auto co_generate_constexpr_eval(expression expr, type_symbol type) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto result_value = this->create_temporary_storage(type);

            auto constructor = submember{.of = type, .name = "CONSTRUCTOR"};
        }

        auto co_generate_expr(block_index& bidx, expression expr) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            co_return co_await rpnx::apply_visitor< typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index > >(
                [&](auto&& val)
                {
                    return co_generate(bidx, std::forward< decltype(val) >(val));
                },
                expr);
        }

        auto co_gen_call_functum(block_index& bidx, type_symbol func, vmir2::invocation_args args) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            std::cout << "co_gen_call_functum(" << quxlang::to_string(func) << ")" << quxlang::to_string(args) << std::endl;

            invotype calltype;
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
                auto arg_type = current_type(bidx, arg);
                bool is_alive = slot_alive(bidx, arg);

                std::cout << " arg name=" << name << " index=" << arg << " is_alive=" << is_alive << std::endl;
                if (!is_alive)
                {
                    assert(typeis< nvalue_slot >(arg_type));
                }
                calltype.named[name] = arg_type;
            }

            initialization_reference functanoid_unnormalized{.initializee = func, .parameters = calltype};

            std::cout << "co_gen_call_functum B(" << quxlang::to_string(functanoid_unnormalized) << ")" << quxlang::to_string(args) << std::endl;
            // Get call type
            auto instanciation = co_await prv.instanciation(functanoid_unnormalized);

            if (!instanciation)
            {
                std::string message = "Cannot call " + to_string(func) + " with " + quxlang::to_string(calltype);
                throw std::logic_error(message);
            }

            co_return co_await this->co_gen_call_functanoid(bidx, instanciation.value(), args);
        }

      private:
        auto current_type(block_index bidx, storage_index idx) -> type_symbol
        {
            auto& block = state.blocks.at(bidx);
            auto& slot_state = block.current_state[idx];
            auto& slot = state.genvalues.at(idx);

            if (slot.template type_is< codegen_binding >())
            {
                auto& binding = slot.template get_as< codegen_binding >();
                return bound_type_reference{.carried_type = current_type(bidx, binding.bound_value), .bound_symbol = binding.bound_symbol};
            }

            auto type = slot.template get_as< codegen_local >().type;

            // NValue and DValue types appear only in the parameter types, the locals
            // are never nvalue or dvalue slots.
            assert(!type.template type_is< nvalue_slot >() && !type.template type_is< dvalue_slot >());

            if (!slot_state.alive)
            {
                type = create_nslot(type);
            }

            return type;
        }

        storage_index index_binding(storage_index idx)
        {
            if (idx == 0)
            {
                return 0;
            }
            codegen_value& codegen_value = this->state.genvalues.at(idx);
            if (!codegen_value.template type_is< codegen_binding >())
            {
                // Locals/literals return as-is
                return idx;
            }

            auto bound_to_index = codegen_value.template get_as< codegen_binding >().bound_value;

            return index_binding(bound_to_index);
        }

        auto create_temporary_storage(type_symbol type) -> vmir2::storage_index
        {
            codegen_local storage;
            storage.type = type;
            this->state.genvalues.push_back(storage);
            return this->state.genvalues.size() - 1;
        }

        auto create_binding(storage_index bindval, type_symbol bind_type)
        {
            codegen_binding binding;
            binding.bound_symbol = bind_type;
            binding.bound_value = bindval;
            this->state.genvalues.push_back(binding);
        }

        void emit(block_index& bidx, vmir2::vm_instruction val)
        {
            codegen_block& block = this->state.blocks.at(bidx);

            auto& state = block.current_state;
            vmir2::state_engine(state, this->state.slots).apply(val);
            auto& new_state = block;
        }

        auto slot_alive(block_index& blk, storage_index slot)
        {
            return state.blocks.at(blk).current_state[slot].alive;
        }

        auto create_reference_internal(block_index& bidx, vmir2::storage_index index, type_symbol const& new_type)
        {
            // This function is used to handle the case where we have an index and need to force it into a
            // reference type.
            // This is mainly used in three places, implied ctor "THIS" argument, dtor, and when a symbol
            // is encountered during an expression.
            auto ty = this->current_type(index);

            vmir2::storage_index temp = create_temporary_storage(bidx, new_type);

            vmir2::make_reference ref;
            ref.value_index = index;
            ref.reference_index = temp;

            this->emit(bidx, ref);

            return temp;
        }

        auto copy_refernece_internal(vmir2::storage_index index)
        {
            // This function is used to handle the case where we have an index and need to force it into a
            // reference type.
            // This is mainly used in three places, implied ctor "THIS" argument, dtor, and when a symbol
            // is encountered during an expression.
            auto ty = this->current_type(index);

            vmir2::storage_index temp = create_temporary_storage(ty);

            vmir2::copy_reference ref;
            ref.from_index = index;
            ref.to_index = temp;

            this->emit(ref);
            return temp;
        }

        auto co_lookup_symbol(type_symbol sym) -> typename CoroutineProvider::template co_type< std::optional< vmir2::storage_index > >
        {
            std::string symbol_str = to_string(sym);

            bool a = typeis< subsymbol >(sym);
            bool b;
            if (typeis< subsymbol >(sym))
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
                    else
                    {
                        lookup = copy_refernece_internal(*lookup);
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

        auto gen_call_ctor(block_index& bidx, type_symbol new_type, vmir2::invocation_args args) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto ctor = submember{.of = new_type, .name = "CONSTRUCTOR"};
            auto new_object = create_temporary_storage(new_type);
            args.named["THIS"] = new_object;
            auto retval = co_await gen_call_functum(bidx, ctor, args);

            assert(retval == 0);

            auto dtor = co_await prv.class_default_dtor(new_type);
            std::cout << "gen_call_ctor A(" << quxlang::to_string(new_type) << ") dtor" << (dtor ? "Y" : "N") << std::endl;
            if (dtor)
            {
                co_await gen_defer_dtor(bidx, retval, submember{.of = new_type, .name = "DESTRUCTOR"}, vmir2::invocation_args{.named = {{"THIS", retval}}});
            }
            co_return new_object;
        }

        auto generate(block_index& bidx, expression_call call) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            std::cout << "gen_call_expr A()" << std::endl;
            auto callee = co_await generate_expr(bidx, call.callee);

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
                auto arg_val_idx = co_await generate_expr(bidx, arg.value);

                // This is probably actually useless?
                // TODO: Test if this does anything
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

            if (bound_symbol_kind == symbol_kind::class_)
            {
                if (!typeis< void_type >(as< bound_type_reference >(callee_type).carried_type))
                {
                    throw std::logic_error("this is bug...");
                }
                auto object_type = as< bound_type_reference >(callee_type).bound_symbol;

                auto val = co_await gen_call_ctor(object_type, args);

                co_return val;
            }

            co_return co_await gen_call_functum(as< bound_type_reference >(callee_type).bound_symbol, args);
        }

      public:
        auto gen_defer_dtor(block_index& bidx, storage_index val, type_symbol dtor, vmir2::invocation_args args) -> typename CoroutineProvider::template co_type< void >
        {
            co_return;
            // TODO: Maybe re-add this later, for now, don't use for default dtors.
            /*
            vmir2::defer_nontrivial_dtor defer;
            defer.on_value = val;
            defer.func = dtor;
            defer.args = args;
            this->emit(defer);
            co_return;
             */
        }

        auto create_numeric_literal(std::string str)
        {
            return exec.create_numeric_literal(str);
        }

        auto create_bool_value(bool val)
        {
            auto boolv = exec.create_temporary(bool_type{});
            vmir2::load_const_int lci;
            lci.value = val ? "1" : "0";
            lci.target = boolv;
            emit(lci);
            return boolv;
        }

        auto gen_call_functanoid(block_index& bidx, instanciation_reference what, vmir2::invocation_args expression_args) -> typename CoroutineProvider::template co_type< vmir2::storage_index >
        {
            std::cout << "gen_call_functanoid(" << quxlang::to_string(what) << ")" << quxlang::to_string(expression_args) << std::endl;
            auto const& call_args_types = what.params;

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
                    co_await gen_call_functum(bidx, arg_final_ctor_func, ctor_args);

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
                    co_await gen_call_functum(bidx, arg_final_ctor_func, ctor_args);

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
                    auto new_index = co_await gen_implicit_conversion(bidx, arg_expr_index, arg_target_type);

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
                exec.current_slot_states[return_slot];

                // calltype.named_parameters["RETURN"] = return_slot_type;
                invocation_args.named["RETURN"] = return_slot;

                retval = return_slot;
            }

            //  assert(what.parameters.size() == args.size());

            if (invocation_args.named.contains("RETURN"))
            {
                assert(invocation_args.size() == what.params.size() + 1);
            }
            else
            {
                assert(invocation_args.size() == what.params.size());
            }

            co_await gen_invoke(bidx, what, invocation_args);

            co_return retval;
        }

        auto co_gen_reinterpret_reference(block_index& bidx, storage_index ref_index, type_symbol target_ref_type) -> typename CoroutineProvider::template co_type< storage_index >
        {
            auto ref_type = this->current_type(ref_index);

            std::string ref_type_str = to_string(ref_type);
            std::string target_ref_type_str = to_string(target_ref_type);

            if (!is_ref(target_ref_type) || !is_ref(ref_type))
            {
                throw std::logic_error("Cannot gen_reinterpret_reference reinterpret non-reference types");
            }

            auto new_index = this->create_temporary_storage(block, target_ref_type);

            vmir2::cast_reference csr;
            csr.source_ref_index = ref_index;
            csr.target_ref_index = new_index;

            this->emit(bidx, csr);

            co_return new_index;
        }

        auto co_gen_reference_conversion(block_index& bidx, storage_index value_index, type_symbol target_reference_type) -> typename CoroutineProvider::template co_type< storage_index >
        {
            // TODO: Support dynamic/static casts
            co_return co_await co_gen_reinterpret_reference(bidx, value_index, target_reference_type);
        }

        auto co_gen_value_conversion(block_index& bidx, storage_index value_index, type_symbol target_value_type) -> typename CoroutineProvider::template co_type< storage_index >
        {
            // TODO: support conversion other than via constructor.
            co_return co_await gen_value_constructor_conversion(bidx, value_index, target_value_type);
        }

        auto co_gen_value_constructor_conversion(block_index& bidx, storage_index value_index, type_symbol target_value_type) -> typename CoroutineProvider::template co_type< storage_index >
        {
            type_symbol value_type = this->current_type(value_index);
            std::cout << "gen_value_conversion(" << value_index << "(" << to_string(value_type) << "), " << to_string(target_value_type) << ")" << std::endl;

            auto new_value_index = create_temporary_storage(target_value_type);

            auto conversion_functum = submember{target_value_type, "CONSTRUCTOR"};

            vmir2::invocation_args args = {.named = {{"THIS", new_value_index}, {"OTHER", value_index}}};

            co_return co_await gen_call_functum(bidx, conversion_functum, args);
        }

        auto co_gen_implicit_conversion(block_index& bidx, storage_index value_index, type_symbol target_type) -> typename CoroutineProvider::template co_type< storage_index >
        {
            type_symbol value_type = this->current_type(value_index);
            std::cout << "gen_implicit_conversion(" << value_index << "(" << to_string(value_type) << "), " << to_string(target_type) << ")" << std::endl;

            if (value_type == target_type)
            {
                co_return value_index;
            }

            if (is_ref(target_type))
            {
                if (is_ref(value_type))
                {
                    co_return co_await gen_reference_conversion(bidx, value_index, target_type);
                }
                else
                {
                    auto temp_index = create_reference_internal(bidx, value_index, make_tref(value_type));
                    co_return co_await gen_reference_conversion(bidx, temp_index, target_type);
                }
            }
            else
            {
                co_return co_await gen_value_conversion(bidx, value_index, target_type);
            }
        }

        auto co_gen_invoke_builtin(block_index& bidx, instanciation_reference what, vmir2::invocation_args const& args) -> typename CoroutineProvider::template co_type< void >
        {
            auto callee = what.temploid;

            auto functum = callee.templexoid;

            intrinsic_builtin_classifier classifier{prv.output_info(), exec};

            auto intrinsic = classifier.intrinsic_instruction(what, args);
            if (intrinsic.has_value())
            {
                this->emit(bidx, intrinsic.value());
                co_return;
            }

            std::string what_str = to_string(what);
            std::string args_str = to_string(args);
            vmir2::invoke ivk;
            ivk.what = what;
            ivk.args = args;

            this->emit(bidx, ivk);

            co_return;
        }

        auto co_gen_invoke(block_index& bidx, instanciation_reference what, vmir2::invocation_args args) -> typename CoroutineProvider::template co_type< void >
        {
            auto is_builtin = co_await prv.function_builtin(what.temploid);
            if (is_builtin)
            {
                co_return co_await gen_invoke_builtin(bidx, what, args);
            }

            if (args.named.contains("RETURN"))
            {
                assert(args.size() == what.params.size() + 1);
            }
            else
            {
                assert(args.size() == what.params.size());
            }
            std::string what_invoke = to_string(what);

            vmir2::invoke ivk;
            ivk.what = what;
            ivk.args = args;

            this->emit(bidx, ivk);
            co_return;
        }

        auto co_generate(block_index& bidx, expression_symbol_reference expr) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            std::string sym = quxlang::to_string(expr.symbol);
            auto value_opt = (co_await this->lookup_symbol(expr.symbol));

            if (!value_opt.has_value())
            {
                throw std::logic_error("Expected symbol " + quxlang::to_string(expr.symbol) + " to be defined.");
            }

            co_return value_opt.value();
        }

        auto co_generate(block_index& bidx, expression_sizeof szof) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            // TODO: implement this.
            throw rpnx::unimplemented();
        }

        auto co_generate(block_index& bidx, expression_this_reference expr) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            throw rpnx::unimplemented();
            co_return 0;
        }

        auto co_generate(block_index& bidx, expression_target target) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            throw rpnx::unimplemented();
        }

        auto co_generate(block_index& bidx, expression_leftarrow expr) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto value = co_await co_generate_expr(bidx, expr.lhs);

            vmir2::make_pointer_to make_pointer;
            make_pointer.of_index = value;

            auto type = this->current_type(value);

            auto non_ref_type = remove_ref(type);

            auto pointer_storage = create_temporary_storage(pointer_type{.target = non_ref_type, .ptr_class = pointer_class::instance, .qual = qualifier::mut});

            make_pointer.pointer_index = pointer_storage;

            this->emit(make_pointer);

            co_return pointer_storage;
        }

        auto co_generate(block_index& bidx, expression_value_keyword const& kw) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            if (kw.keyword == "TRUE")
            {
                co_return this->create_bool_value(true);
            }
            if (kw.keyword == "FALSE")
            {
                co_return this->create_bool_value(false);
            }
            output_info arch = prv.output_info();

            if (kw.keyword == "ARCH_X64")
            {
                co_return this->create_bool_value(arch.cpu_type == cpu::x86_64);
            }
            if (kw.keyword == "ARCH_X86")
            {
                co_return this->create_bool_value(arch.cpu_type == cpu::x86_32);
            }

            if (kw.keyword == "ARCH_ARM32")
            {
                co_return this->create_bool_value(arch.cpu_type == cpu::arm_32);
            }

            if (kw.keyword == "ARCH_ARM64")
            {
                co_return this->create_bool_value(arch.cpu_type == cpu::arm_64);
            }

            if (kw.keyword == "OS_LINUX")
            {
                co_return this->create_bool_value(arch.os_type == os::linux);
            }

            if (kw.keyword == "OS_WINDOWS")
            {
                co_return this->create_bool_value(arch.os_type == os::windows);
            }

            if (kw.keyword == "OS_MACOS")
            {
                co_return this->create_bool_value(arch.os_type == os::macos);
            }

            throw rpnx::unimplemented();
        }

        auto co_generate(block_index& bidx, expression_rightarrow expr) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto value = co_await generate_expr(bidx, expr.lhs);
            auto rightarrow = submember{.of = remove_ref(this->current_type(value)), .name = "OPERATOR->"};
            vmir2::invocation_args args = {.named = {{"THIS", value}}};
            co_return co_await gen_call_functum(rightarrow, args);
            co_return 0;
        }

        auto co_generate(block_index& bidx, expression_binary input) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto lhs = co_await generate_expr(bidx, input.lhs);
            auto rhs = co_await generate_expr(bidx, input.rhs);

            type_symbol lhs_type = this->current_type(lhs);
            type_symbol rhs_type = this->current_type(rhs);

            type_symbol lhs_underlying_type = remove_ref(lhs_type);
            type_symbol rhs_underlying_type = remove_ref(rhs_type);

            type_symbol lhs_function = submember{lhs_underlying_type, "OPERATOR" + input.operator_str};
            type_symbol rhs_function = submember{rhs_underlying_type, "OPERATOR" + input.operator_str + "RHS"};
            invotype lhs_param_info{.named = {{"THIS", lhs_type}, {"OTHER", rhs_type}}};
            invotype rhs_param_info{.named = {{"THIS", rhs_type}, {"OTHER", lhs_type}}};

            auto lhs_exists_and_callable_with = co_await prv.instanciation(initialization_reference{.initializee = lhs_function, .parameters = lhs_param_info});

            if (lhs_exists_and_callable_with)
            {
                auto lhs_args = vmir2::invocation_args{.named = {{"THIS", lhs}, {"OTHER", rhs}}};
                co_return co_await gen_call_functum(lhs_function, lhs_args);
            }

            auto rhs_exists_and_callable_with = co_await prv.instanciation(initialization_reference{.initializee = rhs_function, .parameters = rhs_param_info});

            if (rhs_exists_and_callable_with)
            {
                auto rhs_args = vmir2::invocation_args{.named = {{"THIS", rhs}, {"OTHER", lhs}}};
                co_return co_await co_gen_call_functum(rhs_function, rhs_args);
            }

            throw std::logic_error("Found neither " + to_string(lhs_function) + " callable with (" + to_string(lhs_type) + ", " + to_string(rhs_type) + ") nor " + to_string(rhs_function) + " callable with (" + to_string(rhs_type) + ", " + to_string(lhs_type) + ")");
        }

        auto co_generate(block_index& bidx, expression_numeric_literal input) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto val = this->create_numeric_literal(input.value);
            assert(val != 0);
            auto val_type = this->current_type(val);
            std::cout << "Generated numeric literal " << val << " of type " << to_string(val_type) << std::endl;
            co_return val;
        }

        auto co_generate(block_index& bidx, expression_unary_postfix input) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto val = co_await generate_expr(bidx, input.lhs);
            auto oper = this->get_class_member(val, "OPERATOR" + input.operator_str);
            co_return co_await gen_call_functum(oper, vmir2::invocation_args{.named = {{"THIS", val}}});
        }

        auto co_generate(block_index& bidx, expression_unary_prefix input) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto val = co_await generate_expr(bidx, input.rhs);
            auto oper = this->get_class_member(val, "OPERATOR" + input.operator_str + "PREFIX");
            co_return co_await gen_call_functum(oper, vmir2::invocation_args{.named = {{"THIS", val}}});
        }

        auto co_generate(block_index& bidx, expression_multibind const& what) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto lhs_val = co_await generate_expr(bidx, what.lhs);

            vmir2::invocation_args invoke_brackets_args;
            invoke_brackets_args.named["THIS"] = lhs_val;

            for (auto& arg : what.bracketed)
            {
                invoke_brackets_args.positional.push_back(co_await generate_expr(bidx, arg));
            }

            type_symbol lhs_class_type = this->current_type(lhs_val);
            lhs_class_type = remove_ref(lhs_class_type);
            auto call_brackets_operator = submember{lhs_class_type, what.operator_str};

            co_return co_await gen_call_functum(call_brackets_operator, invoke_brackets_args);
        }

        auto get_class_member(storage_index val, std::string func)
        {
            auto val_type = this->current_type(val);
            auto val_class = remove_ref(val_type);
            auto func_ref = submember{val_class, func};
            return func_ref;
        }

        auto co_generate(block_index& bidx, expression_thisdot_reference what) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto this_reference = subsymbol{.of = context_reference{}, .name = "THIS"};
            auto value = co_await this->co_lookup_symbol(this_reference);
            if (!value)
            {
                throw std::logic_error("Cannot find " + to_string(this_reference));
            }
            auto field = co_await generate_dot_access(bidx, *value, what.field_name);
            co_return field;
        }

        auto co_generate(block_index& bidx, expression_string_literal expr) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            throw rpnx::unimplemented();
        }

        auto co_generate(block_index& bidx, expression_dotreference what) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto parent = co_await co_generate_expr(bidx, what.lhs);
            co_return co_await co_generate_dot_access(bidx, parent, what.field_name);
        }

        auto co_generate_dot_access(block_index& bidx, storage_index base, std::string field_name) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto base_type = this->current_type(base);
            std::string base_type_str = quxlang::to_string(base_type);
            auto base_type_noref = quxlang::remove_ref(base_type);

            std::string base_type_noref_string = quxlang::to_string(base_type_noref);

            // First try to find a field with this name
            class_layout layout = co_await prv.class_layout(base_type_noref);

            // std::string base_type_str = to_string(base_type);

            for (class_field_info const& field : layout.fields)
            {
                if (field.name == field_name)
                {
                    vmir2::access_field access;
                    access.base_index = base;
                    access.field_name = field.name;
                    type_symbol result_ref_type = recast_reference(base_type.template get_as< pointer_type >(), field.type);
                    access.store_index = create_temporary_storage(result_ref_type);
                    std::cout << "Created field access " << access.store_index << " for " << field_name << " in " << to_string(base_type) << std::endl;

                    this->emit(bidx, access);
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

        auto co_generate_body(block_index& current_block) -> typename CoroutineProvider::template co_type< void >
        {
            auto const& inst = func;

            auto& function_ref = inst.temploid;

            auto function_decl_opt = co_await this->prv.function_declaration(function_ref);
            assert(function_decl_opt.has_value());
            ast2_function_declaration& function_decl = function_decl_opt.value();

            co_await generate_function_block(current_block, function_decl.definition.body, "body");

            if (state.blocks.at(current_block).terminator.has_value() == false)
            {
                // TODO: Check if default return is allowed.
                co_await this->generate_return(current_block);
            }

            co_return;
        }

        auto co_generate_dtor_references() -> typename CoroutineProvider::template co_type< void >
        {
            // Loop through all local slots and check if they have non-trivial dtors, then add
            // dtor references to non_trivial_dtors if they do.
            for (codegen_value const& genvalue : state.genvalues)
            {
                if (!genvalue.template type_is< codegen_local >())
                {
                    continue;
                }
                auto& slot = genvalue.template get_as< codegen_local >();
                auto dtor = co_await prv.class_default_dtor(slot.type);
                if (dtor.has_value())
                {
                    assert(!this->state.non_trivial_dtors.contains(slot.type) || state.non_trivial_dtors[slot.type] == *dtor);
                    state.non_trivial_dtors[slot.type] = *dtor;
                }
            }

            co_return;
        }

        auto generate_subblock(block_index& current_block, std::string block_from) -> block_index
        {
            state.blocks.emplace_back();

            codegen_block& new_block = state.blocks.back();
            codegen_block& current_block_ref = state.blocks.at(current_block);

            new_block.entry_state = current_block_ref.current_state;

            return state.blocks.size() - 1;
        }

        [[nodiscard]] auto co_generate_function_block(block_index& current_block, function_block const& block, std::string block_from) -> typename CoroutineProvider::template co_type< void >
        {
            assert(!state.blocks(current_block).terminator.has_value());
            auto new_block = this->generate_subblock(current_block, block_from + "_block_new");

            assert(!state.blocks.at(new_block).terminator.has_value());

            auto after_block = this->generate_subblock(current_block, block_from + "_block_after");
            assert(!state.blocks.at(after_block).terminator.has_value());

            this->generate_jump(current_block, new_block);

            for (auto const& statement : block.statements)
            {
                assert(!this->state.blocks.at(new_block).terminator.has_value());
                co_await generate_fblock_statement(new_block, statement);
            }

            if (!state.blocks.at(new_block).terminator.has_value())
            {
                this->generate_jump(new_block, after_block);
            }

            assert(state.blocks.at(current_block).terminator.has_value());
            current_block = after_block;
            assert(!state.blocks.at(after_block).terminator.has_value());
            co_return;
        }

        void generate_jump(block_index& from, block_index& to)
        {
            auto& from_block = state.blocks.at(from);
            if (from_block.terminator.has_value())
            {
                throw std::logic_error("Cannot jump from a block that already has a terminator");
            }

            vmir2::jump jump_instruction{.target = to};
            from_block.terminator = jump_instruction;
        }
    };

} // namespace quxlang

#endif // RPNX_QUXLANG_CO_VMIR_EXPRESSION_EMITTER_HEADER
