// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_RES_EXPR_CODEGEN_VMIR_EXPRESSION_EMITTER_HEADER_GUARD
#define QUXLANG_RES_EXPR_CODEGEN_VMIR_EXPRESSION_EMITTER_HEADER_GUARD

#include "quxlang/ast2/ast2_entity.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/class_field_declaration.hpp"
#include "quxlang/data/class_layout.hpp"
#include "quxlang/data/contextual_type_reference.hpp"
#include "quxlang/data/expression_call.hpp"
#include "quxlang/data/machine.hpp"
#include "quxlang/exception.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/res/implicitly_convertible_to.hpp"
#include "quxlang/res/symbol_type.hpp"
#include "quxlang/vmir2/assembly.hpp"
#include "quxlang/vmir2/state_engine.hpp"
#include "quxlang/vmir2/vmir2.hpp"
#include "rpnx/simple_coroutine.hpp"
#include "rpnx/uint64_base.hpp"
#include <quxlang/macros.hpp>

namespace quxlang
{

    template < typename T >
    concept codegen_co_provider = requires(T t) {
        { std::declval< typename T::template co_type< quxlang::type_symbol > >() };
        { std::declval< typename T::template co_type< quxlang::vmir2::local_index > >() };
    };

    struct deferral_index : public rpnx::uint64_base< deferral_index >
    {
        using rpnx::uint64_base< deferral_index >::uint64_base;
    };

    RPNX_UNIQUE_U64(value_index);

    using block_index = vmir2::block_index;
    using local_index = vmir2::local_index;

    struct codegen_literal;
    struct codegen_local;
    struct codegen_argument;
    struct codegen_block
    {
        std::map< local_index, vmir2::slot_state > entry_state;
        std::map< local_index, vmir2::slot_state > current_state;
        std::vector< vmir2::vm_instruction > instructions;
        std::optional< vmir2::vm_terminator > terminator;
        std::optional< std::string > dbg_name;
        std::map< std::string, value_index > lookup_values;

        RPNX_MEMBER_METADATA(codegen_block, entry_state, current_state, instructions, terminator, dbg_name, lookup_values);
    };

    struct codegen_argument
    {
        type_symbol type;
        std::optional< value_index > index;
    };

    struct codegen_binding
    {
        type_symbol bound_symbol;
        value_index bound_value;

        RPNX_MEMBER_METADATA(codegen_binding, bound_symbol, bound_value);
    };

    struct codegen_local
    {
        local_index local_index;

        RPNX_MEMBER_METADATA(codegen_local, local_index);
    };

    struct codegen_literal
    {
        type_symbol type;
        std::string value;

        RPNX_MEMBER_METADATA(codegen_literal, type, value);
    };

    using codegen_value = rpnx::variant< codegen_binding, codegen_literal, codegen_local >;

    struct codegen_state
    {
        std::vector< codegen_value > genvalues{codegen_binding{.bound_symbol = void_type(), .bound_value = value_index(0)}};
        std::vector< vmir2::local_type > locals{vmir2::local_type{.type = void_type()}};
        std::vector< codegen_block > blocks;
        vmir2::routine_parameters params;
        std::map< type_symbol, type_symbol > non_trivial_dtors;
        std::map< std::string, value_index > codegen_numeric_literals;
        std::map< std::string, value_index > top_level_lookups;
        std::map< std::string, value_index > top_level_lookups_weak;
        type_symbol context;
        std::optional< instanciation_reference > functanoid_type;
    };

    struct codegen_invocation_args
    {
        std::map< std::string, value_index > named;
        std::vector< value_index > positional;

        inline auto size() const
        {
            return positional.size() + named.size();
        }

        RPNX_MEMBER_METADATA(codegen_invocation_args, named, positional);
    };

    template < codegen_co_provider CoroutineProvider >
    class co_vmir_generator
    {

        // The following structs are basically integer types but they can't be interconverted with each other.
        // They are used to represent different indices in the code generation this->state.

      private:
        CoroutineProvider prv;
        codegen_state state;
        type_symbol ctx;

      public:
        co_vmir_generator(CoroutineProvider prv, type_symbol ctx) : prv(prv), ctx(ctx)
        {
        }

        auto co_generate_constexpr_eval(expression expr, type_symbol type) -> typename CoroutineProvider::template co_type< vmir2::functanoid_routine3 >
        {
            assert(this->state.blocks.empty());
            this->state.blocks.push_back(codegen_block{});
            auto current_block = block_index(0);
            std::string type_str = to_string(type);
            std::string expr_str = to_string(expr);
            auto result_val = co_await this->co_generate_typed_expr(current_block, expr, type);
            assert(current_block == block_index(0) || this->state.blocks.at(0).terminator.has_value());
            assert(result_val != value_index(0));
            vmir2::constexpr_set_result csr;
            assert(this->current_type(current_block, result_val) == type);
            csr.target = get_local_index(result_val);
            this->emit(current_block, csr);

            this->generate_return(current_block);

            co_await co_generate_dtor_references();

            co_return get_result();
        }

        bool local_alive(block_index bidx, local_index idx)
        {
            auto& block = this->state.blocks.at(bidx);
            if (block.current_state.contains(idx))
            {
                return block.current_state.at(idx).alive;
            }
            return false;
        }

        bool value_alive(block_index bidx, value_index idx)
        {
            if (state.genvalues.at(idx).template type_is< codegen_literal >())
            {
                return true;
            }
            return local_alive(bidx, get_local_index(idx));
        }

        auto co_generate_expr(block_index& bidx, expression const& expr) -> typename CoroutineProvider::template co_type< value_index >
        {
            assert(bidx == block_index(0) || this->state.blocks.at(0).terminator.has_value());
            auto result = co_await rpnx::apply_visitor< typename CoroutineProvider::template co_type< value_index > >(
                [&](auto&& val)
                {
                    return co_generate(bidx, std::forward< decltype(val) >(val));
                },
                expr);
            std::string expr_str = to_string(expr);
            assert(bidx == block_index(0) || this->state.blocks.at(0).terminator.has_value());
            co_return result;
        }

        auto co_gen_call_functum(block_index& bidx, type_symbol func, codegen_invocation_args args) -> typename CoroutineProvider::template co_type< value_index >
        {
            std::cout << "co_gen_call_functum(" << quxlang::to_string(func) << ")" << quxlang::to_string(args) << std::endl;

            invotype calltype;
            for (auto& arg : args.positional)
            {
                auto arg_type = this->current_type(bidx, arg);
                bool is_alive = this->value_alive(bidx, arg);
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
                bool is_alive = value_alive(bidx, arg);

                std::cout << " arg name=" << name << " index=" << arg << " is_alive=" << is_alive << std::endl;
                if (!is_alive)
                {
                    assert(typeis< nvalue_slot >(arg_type));
                }
                calltype.named[name] = arg_type;
            }

            initialization_reference functanoid_unnormalized{.initializee = func, .parameters = calltype};

            std::cout << "co_gen_call_functum selected: (" << quxlang::to_string(functanoid_unnormalized) << ")" << std::endl;
            //  Get call type
            auto instanciation = co_await prv.instanciation(functanoid_unnormalized);

            if (!instanciation)
            {
                std::string message = "Cannot call " + to_string(func) + " with " + quxlang::to_string(calltype);

                auto functum_overloeads = co_await prv.functum_overloads(func);

                for (auto const& overload : functum_overloeads)
                {
                    message += "\n - Candidate: " + to_string(overload.interface);
                }

                throw std::logic_error(message);
            }

            co_return co_await this->co_gen_call_functanoid(bidx, instanciation.value(), args);
        }

      private:
        auto current_type(block_index bidx, value_index idx) -> type_symbol
        {
            if (idx == 0)
            {
                return void_type();
            }
            auto& block = this->state.blocks.at(bidx);

            auto& slot = this->state.genvalues.at(idx);
            if (slot.template type_is< codegen_literal >())
            {
                return slot.template get_as< codegen_literal >().type;
            }

            if (slot.template type_is< codegen_binding >())
            {
                auto bound_value = slot.template get_as< codegen_binding >().bound_value;
                auto bound_symbol = slot.template get_as< codegen_binding >().bound_symbol;
                assert(!qualified_is_contextual(bound_symbol));
                auto bound_type = this->current_type(bidx, bound_value);
                return bound_type_reference{.carried_type = bound_type, .bound_symbol = bound_symbol};
            }

            auto local_idx = get_local_index(idx);
            auto& slot_state = block.current_state[local_idx];

            if (slot.template type_is< codegen_binding >())
            {
                auto& binding = slot.template get_as< codegen_binding >();
                return bound_type_reference{.carried_type = current_type(bidx, binding.bound_value), .bound_symbol = binding.bound_symbol};
            }

            auto type = state.locals.at(slot.template get_as< codegen_local >().local_index).type;

            // NValue and DValue types appear only in the parameter types, the locals
            // are never nvalue or dvalue slots.
            assert(!type.template type_is< nvalue_slot >() && !type.template type_is< dvalue_slot >());

            if (!slot_state.alive)
            {
                type = create_nslot(type);
            }

            return type;
        }

        value_index genlocal_index(value_index idx)
        {
            if (idx == 0)
            {
                return value_index(0);
            }
            codegen_value& codegen_value = this->state.genvalues.at(idx);
            if (!codegen_value.template type_is< codegen_binding >())
            {
                // Locals/literals return as-is
                return idx;
            }

            auto bound_to_index = codegen_value.template get_as< codegen_binding >().bound_value;

            return genlocal_index(bound_to_index);
        }

        local_index get_local_index(value_index idx)
        {
            idx = genlocal_index(idx);
            if (idx == 0)
            {
                return local_index(0);
            }

            codegen_local& local = this->state.genvalues.at(idx).template get_as< codegen_local >();

            return local.local_index;
        }

        auto create_local_value(type_symbol type) -> value_index
        {
            // Locals cannot be nvalue or dvalue slots, that is only the case for parameters.
            assert(!type.template type_is< nvalue_slot >() && !type.template type_is< dvalue_slot >());
            codegen_local storage;
            this->state.locals.push_back(vmir2::local_type{.type = type});
            storage.local_index = local_index(this->state.locals.size() - 1);
            this->state.genvalues.push_back(storage);
            return value_index(this->state.genvalues.size() - 1);
        }

        auto local_value_direct_lookup(block_index bidx, std::string str) -> std::optional< value_index >
        {
            auto it = this->state.blocks.at(bidx).lookup_values.find(str);
            if (it != this->state.blocks.at(bidx).lookup_values.end())
            {
                return it->second;
            }
            // If we don't find it in the current block, we can look in the top-level lookups.
            auto top_it = this->state.top_level_lookups.find(str);
            if (top_it != this->state.top_level_lookups.end())
            {
                return top_it->second;
            }
            auto weak_it = this->state.top_level_lookups_weak.find(str);
            if (weak_it != this->state.top_level_lookups_weak.end())
            {
                return weak_it->second;
            }
            return std::nullopt;
        }

        auto create_binding(value_index bindval, type_symbol bind_type)
        {
            assert(!qualified_is_contextual(bind_type));
            codegen_binding binding;
            binding.bound_symbol = bind_type;
            binding.bound_value = bindval;
            this->state.genvalues.push_back(binding);
            return value_index(this->state.genvalues.size() - 1);
        }

        void emit(block_index& bidx, vmir2::vm_instruction val)
        {
            codegen_block& block = this->state.blocks.at(bidx);
            // val.from = get_local_index(val.from);
            // ... (do this for all relevant fields)
            vmir2::codegen_state_engine(this->state.blocks.at(bidx).current_state, this->state.locals, this->state.params).apply(val);

            state.blocks.at(bidx).instructions.push_back(val);
        }

        auto create_reference_internal(block_index& bidx, value_index index, type_symbol const& new_type)
        {
            // This function is used to handle the case where we have an index and need to force it into a
            // reference type.
            // This is mainly used in three places, implied ctor "THIS" argument, dtor, and when a symbol
            // is encountered during an expression.
            auto ty = this->current_type(bidx, index);

            value_index temp = create_local_value(new_type);

            vmir2::make_reference ref;
            ref.value_index = get_local_index(index);
            ref.reference_index = get_local_index(temp);

            this->emit(bidx, ref);

            return temp;
        }

        auto copy_refernece_internal(block_index bidx, value_index index)
        {
            // This function is used to handle the case where we have an index and need to force it into a
            // reference type.
            // This is mainly used in three places, implied ctor "THIS" argument, dtor, and when a symbol
            // is encountered during an expression.
            auto ty = this->current_type(bidx, index);

            auto temp = create_local_value(ty);

            vmir2::copy_reference ref;
            ref.from_index = get_local_index(index);
            ref.to_index = get_local_index(temp);

            this->emit(bidx, ref);
            return temp;
        }

        auto co_lookup_symbol(block_index idx, type_symbol sym) -> typename CoroutineProvider::template co_type< std::optional< value_index > >
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

            if (typeis< freebound_identifier >(sym))
            {
                std::string const& name = as< freebound_identifier >(sym).name;
                //std::cout << "lookup " << name << std::endl;
                auto lookup = this->local_value_direct_lookup(idx, name);
                if (lookup)
                {
                    auto lookup_type = this->current_type(idx, lookup.value());
                    //std::cout << "lookup " << name << " -> " << lookup.value() << " type=" << to_string(lookup_type) << std::endl;

                    if (!is_ref(lookup_type))
                    {
                        lookup = create_reference_internal(idx, *lookup, make_mref(lookup_type));
                    }
                    else
                    {
                        lookup = copy_refernece_internal(idx, *lookup);
                    }

                    assert(!qualified_is_contextual(lookup_type));

                    co_return lookup;
                }
            }
            auto canonical_symbol_opt = co_await prv.lookup(contextual_type_reference{.context = ctx, .type = sym});

            if (!canonical_symbol_opt)
            {
                co_return std::nullopt;
            }

            assert(!qualified_is_contextual(canonical_symbol_opt.value()));

            auto canonical_symbol = canonical_symbol_opt.value();

            auto kind = co_await prv.symbol_type(canonical_symbol);

            value_index index(0);

            auto binding = this->create_binding(value_index(0), canonical_symbol);

            if (kind == quxlang::symbol_kind::global_variable)
            {
                auto variable_type = co_await prv.variable_type(canonical_symbol);
                throw rpnx::unimplemented();
            }
            else
            {
                index = binding;
            }

            co_return index;
        }

        auto co_gen_idx_conversion(value_index idx, type_symbol to_type) -> typename CoroutineProvider::template co_type< value_index >
        {
            codegen_invocation_args args;
            args.named["OTHER"] = idx;
            co_return co_await co_gen_call_ctor(to_type, std::move(args));
        }

        auto co_gen_call_ctor(block_index& bidx, type_symbol new_type, codegen_invocation_args args) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto ctor = submember{.of = new_type, .name = "CONSTRUCTOR"};
            auto new_object = create_local_value(new_type);
            args.named["THIS"] = new_object;
            auto retval = co_await co_gen_call_functum(bidx, ctor, args);

            assert(retval == 0);

            auto dtor = co_await prv.class_default_dtor(new_type);
            std::cout << "gen_call_ctor A(" << quxlang::to_string(new_type) << ") dtor" << (dtor ? "Y" : "N") << std::endl;
            if (dtor)
            {
                this->add_nontrivial_default_dtor(new_type, *dtor);
            }
            co_return new_object;
        }

        auto co_generate(block_index& bidx, expression_call call) -> typename CoroutineProvider::template co_type< value_index >
        {
            std::cout << "gen_call_expr A()" << std::endl;
            auto callee = co_await co_generate_expr(bidx, call.callee);

            type_symbol callee_type = this->current_type(bidx, callee);
            std::cout << "gen_call_expr B() -> callee_type=" << quxlang::to_string(callee_type) << std::endl;

            std::string callee_type_string = to_string(callee_type);

            if (!typeis< bound_type_reference >(callee_type))
            {
                auto value_type = remove_ref(callee_type);
                auto operator_call = submember{.of = value_type, .name = "OPERATOR()"};
                callee = this->create_binding(callee, operator_call);
                callee_type = this->current_type(bidx, callee);
            }

            type_symbol bound_symbol = as< bound_type_reference >(callee_type).bound_symbol;

            type_symbol carried_type = as< bound_type_reference >(callee_type).carried_type;
            symbol_kind bound_symbol_kind = co_await prv.symbol_type(bound_symbol);

            codegen_invocation_args args;
            std::string callee_type_string2 = to_string(as< bound_type_reference >(callee_type));

            std::cout << "requesting generate call to bindval=" << to_string(carried_type) << " bindsym=" << to_string(bound_symbol) << std::endl;

            std::string callee_type_string3 = to_string(callee_type);

            for (auto& arg : call.args)
            {
                auto arg_val_idx = co_await co_generate_expr(bidx, arg.value);

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
                args.named["THIS"] = callee;
            }

            if (bound_symbol_kind == symbol_kind::class_)
            {
                if (!typeis< void_type >(as< bound_type_reference >(callee_type).carried_type))
                {
                    throw std::logic_error("this is bug...");
                }
                auto object_type = as< bound_type_reference >(callee_type).bound_symbol;

                auto val = co_await co_gen_call_ctor(bidx, object_type, args);

                co_return val;
            }

            co_return co_await co_gen_call_functum(bidx, as< bound_type_reference >(callee_type).bound_symbol, args);
        }

      public:
        auto co_gen_defer_dtor(block_index& bidx, value_index val, type_symbol dtor, codegen_invocation_args args) -> typename CoroutineProvider::template co_type< void >
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
            if (auto it = this->state.codegen_numeric_literals.find(str); it != this->state.codegen_numeric_literals.end())
            {
                return it->second;
            }
            codegen_literal lit;
            lit.type = numeric_literal_reference{};
            lit.value = str;
            this->state.genvalues.push_back(lit);
            this->state.codegen_numeric_literals[str] = value_index(this->state.genvalues.size() - 1);
            return value_index(this->state.genvalues.size() - 1);
        }

        auto create_bool_value(block_index bidx, bool val) -> value_index
        {
            auto boolv = this->create_local_value(bool_type{});
            vmir2::load_const_bool lcb;
            lcb.value = val;
            lcb.target = get_local_index(boolv);
            emit(bidx, lcb);
            return boolv;
        }

        auto co_gen_call_functanoid(block_index& bidx, instanciation_reference what, codegen_invocation_args expression_args) -> typename CoroutineProvider::template co_type< value_index >
        {
            //  std::cout << "gen_call_functanoid(" << quxlang::to_string(what) << ")" << quxlang::to_string(expression_args) << std::endl;
            auto const& call_args_types = what.params;

            // TODO: Support defaulted parameters.

            codegen_invocation_args invocation_args;

            auto create_arg_value = [&](value_index arg_expr_index, type_symbol arg_target_type) -> typename CoroutineProvider::template co_type< value_index >
            {
                // TODO: Support PRValue args
                auto arg_expr_type = this->current_type(bidx, arg_expr_index);
                bool arg_alive = this->value_alive(bidx, arg_expr_index);

                if (!arg_alive)
                {
                    assert(!is_ref(arg_expr_type));
                    // arg_expr_type = nvalue_slot{arg_expr_type};
                }

                if (is_ref(arg_expr_type) && !is_ref(arg_target_type))
                {
                    //  std::cout << "gen_call_functanoid A(" << quxlang::to_string(what) << ")" << quxlang::to_string(arg_expr_type) << "->" << quxlang::to_string(arg_target_type) << quxlang::to_string(expression_args) << std::endl;
                    auto index = create_local_value(arg_target_type);

                    std::cout << "Created argument slot " << index << std::endl;
                    // Alive is false
                    auto arg_final_ctor_func = submember{arg_target_type, "CONSTRUCTOR"};

                    codegen_invocation_args ctor_args = {.named = {{"THIS", index}, {"OTHER", arg_expr_index}}};
                    // These both need to be references or the constructor will probably infinite loop.
                    assert(is_ref(arg_expr_type) && is_ref(make_mref(arg_target_type)));

                    // TODO: instead of directly calling the constructor, call a special conversion function perhaps?
                    co_await co_gen_call_functum(bidx, arg_final_ctor_func, ctor_args);

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

                    auto index = create_local_value(arg_target_type);
                    std::cout << "Created argument slot " << index << std::endl;
                    // Alive is false
                    auto arg_final_ctor_func = submember{arg_target_type, "CONSTRUCTOR"};

                    codegen_invocation_args ctor_args = {.named = {{"THIS", index}, {"OTHER", arg_expr_index}}};

                    // TODO: instead of directly calling the constructor, call a special conversion function perhaps?
                    co_await co_gen_call_functum(bidx, arg_final_ctor_func, ctor_args);

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
                    auto new_index = co_await co_gen_implicit_conversion(bidx, arg_expr_index, arg_target_type);

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

            assert(!qualified_is_contextual(what));
            auto return_type = co_await prv.functanoid_return_type(what);

            // Index 0 is defined to be the special "void" value.
            value_index retval(0);

            if (!typeis< void_type >(return_type))
            {
                auto return_slot = create_local_value(return_type);
                std::cout << "Created return slot " << return_slot << std::endl;

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

            co_await co_gen_invoke(bidx, what, invocation_args);

            co_return retval;
        }

        auto co_gen_reinterpret_reference(block_index& bidx, value_index ref_index, type_symbol target_ref_type) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto ref_type = this->current_type(bidx, ref_index);

            std::string ref_type_str = to_string(ref_type);
            std::string target_ref_type_str = to_string(target_ref_type);

            if (!is_ref(target_ref_type) || !is_ref(ref_type))
            {
                throw std::logic_error("Cannot gen_reinterpret_reference reinterpret non-reference types");
            }

            auto new_index = this->create_local_value(target_ref_type);

            vmir2::cast_reference csr;
            csr.source_ref_index = get_local_index(ref_index);
            csr.target_ref_index = get_local_index(new_index);

            this->emit(bidx, csr);

            co_return new_index;
        }

        auto co_gen_reference_conversion(block_index& bidx, value_index vidx, type_symbol target_reference_type) -> typename CoroutineProvider::template co_type< value_index >
        {
            // TODO: Support dynamic/static casts
            co_return co_await co_gen_reinterpret_reference(bidx, vidx, target_reference_type);
        }

        auto co_gen_value_conversion(block_index& bidx, value_index vidx, type_symbol target_value_type) -> typename CoroutineProvider::template co_type< value_index >
        {
            // TODO: support conversion other than via constructor.
            co_return co_await co_gen_value_constructor_conversion(bidx, vidx, target_value_type);
        }

        auto co_gen_value_constructor_conversion(block_index& bidx, value_index vidx, type_symbol target_value_type) -> typename CoroutineProvider::template co_type< value_index >
        {
            type_symbol value_type = this->current_type(bidx, vidx);
            std::cout << "co_gen_value_conversion(" << vidx << "(" << to_string(value_type) << "), " << to_string(target_value_type) << ")" << std::endl;

            auto new_value_index = create_local_value(target_value_type);

            auto conversion_functum = submember{target_value_type, "CONSTRUCTOR"};

            codegen_invocation_args args = {.named = {{"THIS", new_value_index}, {"OTHER", vidx}}};

            co_await co_gen_call_functum(bidx, conversion_functum, args);
            co_return new_value_index;
        }

        auto co_gen_implicit_conversion(block_index& bidx, value_index vidx, type_symbol target_type) -> typename CoroutineProvider::template co_type< value_index >
        {
            type_symbol value_type = this->current_type(bidx, vidx);
            std::cout << "gen_implicit_conversion(" << vidx << "(" << to_string(value_type) << "), " << to_string(target_type) << ")" << std::endl;

            if (value_type == target_type)
            {
                assert(vidx != value_index(0));
                co_return vidx;
            }

            if (is_ref(target_type))
            {
                if (is_ref(value_type))
                {
                    co_return co_await co_gen_reference_conversion(bidx, vidx, target_type);
                }
                else
                {
                    auto temp_index = create_reference_internal(bidx, vidx, make_tref(value_type));
                    co_return co_await co_gen_reference_conversion(bidx, temp_index, target_type);
                }
            }
            else
            {
                co_return co_await co_gen_value_conversion(bidx, vidx, target_type);
            }
        }

        auto co_gen_invoke_builtin(block_index& bidx, instanciation_reference what, codegen_invocation_args const& args) -> typename CoroutineProvider::template co_type< void >
        {
            auto callee = what.temploid;

            auto functum = callee.templexoid;

            auto intrinsic = this->intrinsic_instruction(what, args);
            if (intrinsic.has_value())
            {
                this->emit(bidx, intrinsic.value());
                co_return;
            }

            std::string what_str = to_string(what);
            // std::string args_str = to_string(args);
            vmir2::invoke ivk;
            ivk.what = what;
            ivk.args = get_invocation_args(args);

            this->emit(bidx, ivk);

            co_return;
        }

        bool is_intrinsic_type(type_symbol of_type)
        {
            return of_type.type_is< int_type >() || of_type.type_is< bool_type >() || of_type.type_is< pointer_type >() || of_type.type_is< array_type >();
        }

        // This implements builtin operators for primitives,
        // It assumes that we have already checked that the function is builtin and are just
        // checking which implementation to use.
        template < typename Inst >
        bool implement_binary_instruction(std::optional< vmir2::vm_instruction >& out, std::string const& operator_str, bool enable_rhs, submember const& member, invotype const& call, codegen_invocation_args const& args, bool flip = false)
        {
            if (member.name == "OPERATOR" + operator_str || (member.name == "OPERATOR" + operator_str + "RHS" && enable_rhs))
            {

                if (call.named.contains("THIS") && call.named.contains("OTHER") && args.size() == 3)
                {
                    auto this_slot_id = get_local_index(args.named.at("THIS"));
                    auto other_slot_id = get_local_index(args.named.at("OTHER"));

                    Inst instr{};

                    instr.a = this_slot_id;
                    instr.b = other_slot_id;
                    if (flip)
                    {
                        std::swap(instr.a, instr.b);
                    }
                    instr.result = get_local_index(args.named.at("RETURN"));

                    out = instr;
                    return true;
                }
            }
            return false;
        }

        std::optional< vmir2::vm_instruction > intrinsic_instruction(type_symbol func, codegen_invocation_args args)
        {
            std::string funcname = to_string(func);

            auto is_arry_pointer = [](type_symbol const& type)
            {
                if (type.type_is< pointer_type >())
                {
                    auto const& ptr = type.as< pointer_type >();
                    return ptr.ptr_class == pointer_class::array;
                }
                return false;
            };
            auto has_incdec_operation_with_incdec_ir = [&](type_symbol const& type)
            {
                return typeis< int_type >(type) || is_arry_pointer(type);
            };

            if (funcname == "[4] I64::.OPERATOR[] #{@THIS CONST& [4] I64, U64}")
            {
                int debugpoint = 0;
            }
            auto cls = func_class(func);
            if (!cls)
            {
                return std::nullopt;
            }

            if (!is_intrinsic_type(*cls))
            {
                return std::nullopt;
            }

            auto instanciation = func.cast_ptr< instanciation_reference >();
            assert(instanciation);

            auto selection = &instanciation->temploid;
            assert(selection);

            auto member = selection->templexoid.cast_ptr< submember >();
            assert(member);

            auto const& call = instanciation->params;

            if (member->name == "OPERATOR??")
            {
                if (cls->template type_is< pointer_type >() && cls->as< pointer_type >().ptr_class != pointer_class::ref)
                {
                    if (args.named.contains("THIS") && args.named.contains("RETURN") && args.size() == 2)
                    {
                        auto this_slot_id = args.named.at("THIS");

                        vmir2::to_bool tb{};
                        tb.from = get_local_index(this_slot_id);
                        tb.to = get_local_index(args.named.at("RETURN"));

                        return tb;
                    }
                }
            }

            if (member->name == "OPERATOR++" && has_incdec_operation_with_incdec_ir(*cls))
            {
                if (call.named.contains("THIS") && call.size() == 1)
                {
                    auto this_slot_id = args.named.at("THIS");

                    vmir2::increment inc{};
                    inc.value = get_local_index(this_slot_id);
                    inc.result = get_local_index(args.named.at("RETURN"));
                    return inc;
                }
            }
            else if (member->name == "OPERATOR++RHS" && has_incdec_operation_with_incdec_ir(*cls))
            {
                if (call.named.contains("THIS") && call.size() == 1)
                {
                    auto this_slot_id = args.named.at("THIS");

                    vmir2::preincrement preinc{};
                    preinc.target = get_local_index(this_slot_id);
                    preinc.target2 = get_local_index(args.named.at("RETURN"));
                    return preinc;
                }
            }

            if (member->name == "OPERATOR--" && has_incdec_operation_with_incdec_ir(*cls))
            {
                if (call.named.contains("THIS") && call.size() == 1)
                {
                    auto this_slot_id = args.named.at("THIS");

                    vmir2::decrement dec{};
                    dec.value = get_local_index(this_slot_id);
                    dec.result = get_local_index(args.named.at("RETURN"));
                    return dec;
                }
            }
            else if (member->name == "OPERATOR--RHS" && has_incdec_operation_with_incdec_ir(*cls))
            {
                if (call.named.contains("THIS") && call.size() == 1)
                {
                    auto this_slot_id = args.named.at("THIS");

                    vmir2::predecrement predec{};
                    predec.target = get_local_index(this_slot_id);
                    predec.target2 = get_local_index(args.named.at("RETURN"));
                    return predec;
                }
            }

            if (member->name == "OPERATOR[]" || member->name == "OPERATOR[&]")
            {
                if (cls->template type_is< array_type >())
                {
                    if (call.named.contains("THIS") && args.named.contains("RETURN") && call.positional.size() == 1 && args.size() == 3)
                    {
                        auto this_slot_id = args.named.at("THIS");
                        auto index_slot_id = args.positional.at(0);
                        auto return_slot_id = args.named.at("RETURN");

                        vmir2::access_array aca{};
                        aca.base_index = get_local_index(this_slot_id);
                        aca.index_index = get_local_index(index_slot_id);
                        aca.store_index = get_local_index(return_slot_id);

                        return aca;
                    }
                }
            }

            if (member->name == "CONSTRUCTOR")
            {
                if (call.named.contains("OTHER"))
                {
                    auto const& other = call.named.at("OTHER");
                    if (cls->template type_is< int_type >() && other.type_is< numeric_literal_reference >())
                    {
                        auto other_slot_id = args.named.at("OTHER");

                        auto const& other_slot = this->state.genvalues.at(other_slot_id);

                        assert(other_slot.template type_is< codegen_literal >());

                        auto const& other_literal = other_slot.template get_as< codegen_literal >();
                        auto const& other_slot_value = other_literal.value;

                        vmir2::load_const_int result;
                        result.value = other_slot_value;
                        result.target = get_local_index(args.named.at("THIS"));

                        return result;
                    }
                    else if (other == make_cref(*cls))
                    {
                        auto other_slot_id = args.named.at("OTHER");
                        auto this_slot_id = args.named.at("THIS");

                        vmir2::load_from_ref lfr{};
                        lfr.from_reference = get_local_index(other_slot_id);
                        lfr.to_value = get_local_index(this_slot_id);

                        return lfr;
                    }
                }
                else if (args.size() == 1 && args.named.contains("THIS"))
                {
                    vmir2::load_const_zero result{};
                    result.target = get_local_index(args.named.at("THIS"));
                    return result;
                }
            }

            if (member->name == "OPERATOR:=")
            {
                if (cls->template type_is< int_type >() || cls->template type_is< bool_type >() || cls->template type_is< pointer_type >())
                {
                    if (call.named.contains("OTHER") && call.named.contains("THIS") && call.size() == 2)
                    {
                        auto const& other = call.named.at("OTHER");
                        auto const& this_ = call.named.at("THIS");

                        if ((other == *cls) && this_ == make_wref(*cls))
                        {
                            auto other_slot_id = args.named.at("OTHER");
                            auto this_slot_id = args.named.at("THIS");

                            vmir2::store_to_ref mov{};
                            mov.from_value = get_local_index(other_slot_id);
                            mov.to_reference = get_local_index(this_slot_id);

                            return mov;
                        }
                    }
                }
            }
            else if (member->name == "OPERATOR->")
            {
                if (cls->template type_is< pointer_type >())
                {
                    if (call.named.contains("THIS") && args.size() == 2)
                    {

                        auto this_slot_id = args.named.at("THIS");

                        vmir2::dereference_pointer deref{};
                        deref.from_pointer = get_local_index(this_slot_id);
                        deref.to_reference = get_local_index(args.named.at("RETURN"));

                        return deref;
                    }
                }
            }
            else if (cls->template type_is< int_type >())
            {
                std::optional< vmir2::vm_instruction > instr;
                if (implement_binary_instruction< vmir2::int_add >(instr, "+", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::int_sub >(instr, "-", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::int_mul >(instr, "*", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::int_div >(instr, "/", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::int_mod >(instr, "%", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::cmp_eq >(instr, "==", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::cmp_ne >(instr, "!=", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::cmp_lt >(instr, "<", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::cmp_lt >(instr, ">", true, *member, call, args, true))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::cmp_ge >(instr, "<=", true, *member, call, args, true))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::cmp_ge >(instr, ">=", true, *member, call, args))
                {
                    return instr;
                }
            }

            if (cls->template type_is< pointer_type >() && (member->name == "OPERATOR+" || member->name == "OPERATOR-"))
            {
                if (call.named.contains("THIS") && call.named.contains("OTHER") && call.named.at("OTHER").type_is< int_type >() && call.size() == 2)
                {
                    vmir2::pointer_arith par;
                    par.from = get_local_index(args.named.at("THIS"));
                    if (member->name == "OPERATOR-")
                    {
                        par.multiplier = -1;
                    }
                    else
                    {
                        assert(member->name == "OPERATOR+");
                        par.multiplier = 1;
                    }
                    par.offset = get_local_index(args.named.at("OTHER"));
                    par.result = get_local_index(args.named.at("RETURN"));
                    return par;
                }

                if (call.named.contains("THIS") && call.named.contains("OTHER") && call.named.at("OTHER").type_is< pointer_type >() && call.size() == 2)
                {
                    vmir2::pointer_diff pdf;
                    pdf.from = get_local_index(args.named.at("THIS"));
                    pdf.to = get_local_index(args.named.at("OTHER"));
                    pdf.result = get_local_index(args.named.at("RETURN"));
                    return pdf;
                }
            }

            return std::nullopt;
        }

        auto co_gen_invoke(block_index& bidx, instanciation_reference what, codegen_invocation_args args) -> typename CoroutineProvider::template co_type< void >
        {
            auto is_builtin = co_await prv.function_builtin(what.temploid);
            if (is_builtin)
            {
                co_return co_await co_gen_invoke_builtin(bidx, what, args);
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
            ivk.args = get_invocation_args(args);

            this->emit(bidx, ivk);
            co_return;
        }

        auto co_generate(block_index& bidx, expression_symbol_reference expr) -> typename CoroutineProvider::template co_type< value_index >
        {
            std::string sym = quxlang::to_string(expr.symbol);
            auto value_opt = (co_await this->co_lookup_symbol(bidx, expr.symbol));

            if (!value_opt.has_value())
            {
                throw std::logic_error("Expected symbol " + quxlang::to_string(expr.symbol) + " to be defined.");
            }

            co_return value_opt.value();
        }

        auto co_generate(block_index& bidx, expression_sizeof szof) -> typename CoroutineProvider::template co_type< value_index >
        {
            // TODO: implement this.
            throw rpnx::unimplemented();
        }

        auto co_generate(block_index& bidx, expression_this_reference expr) -> typename CoroutineProvider::template co_type< value_index >
        {
            throw rpnx::unimplemented();
            co_return value_index(0);
        }

        auto co_generate(block_index& bidx, expression_target target) -> typename CoroutineProvider::template co_type< value_index >
        {
            throw rpnx::unimplemented();
        }

        auto co_generate(block_index& bidx, expression_leftarrow expr) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto value = co_await co_generate_expr(bidx, expr.lhs);

            vmir2::make_pointer_to make_pointer;
            make_pointer.of_index = get_local_index(value);

            auto type = this->current_type(bidx, value);

            auto non_ref_type = remove_ref(type);

            auto pointer_storage = create_local_value(pointer_type{.target = non_ref_type, .ptr_class = pointer_class::instance, .qual = qualifier::mut});

            make_pointer.pointer_index = get_local_index(pointer_storage);

            this->emit(bidx, make_pointer);

            co_return pointer_storage;
        }

        auto co_generate(block_index& bidx, expression_value_keyword const& kw) -> typename CoroutineProvider::template co_type< value_index >
        {
            if (kw.keyword == "TRUE")
            {
                co_return this->create_bool_value(bidx, true);
            }
            if (kw.keyword == "FALSE")
            {
                co_return this->create_bool_value(bidx, false);
            }
            output_info arch = prv.output_info();

            if (kw.keyword == "ARCH_X64")
            {
                assert(bidx == block_index(0) || this->state.blocks.at(0).terminator.has_value());
                auto result = this->create_bool_value(bidx, arch.cpu_type == cpu::x86_64);
                assert(bidx == block_index(0) || this->state.blocks.at(0).terminator.has_value());
                co_return result;
            }
            if (kw.keyword == "ARCH_X86")
            {
                co_return this->create_bool_value(bidx, arch.cpu_type == cpu::x86_32);
            }

            if (kw.keyword == "ARCH_ARM32")
            {
                co_return this->create_bool_value(bidx, arch.cpu_type == cpu::arm_32);
            }

            if (kw.keyword == "ARCH_ARM64")
            {
                co_return this->create_bool_value(bidx, arch.cpu_type == cpu::arm_64);
            }

            if (kw.keyword == "OS_LINUX")
            {
                co_return this->create_bool_value(bidx, arch.os_type == os::linux);
            }

            if (kw.keyword == "OS_WINDOWS")
            {
                co_return this->create_bool_value(bidx, arch.os_type == os::windows);
            }

            if (kw.keyword == "OS_MACOS")
            {
                co_return this->create_bool_value(bidx, arch.os_type == os::macos);
            }

            throw rpnx::unimplemented();
        }

        auto co_generate(block_index& bidx, expression_rightarrow expr) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto value = co_await co_generate_expr(bidx, expr.lhs);
            auto rightarrow = submember{.of = remove_ref(this->current_type(bidx, value)), .name = "OPERATOR->"};
            codegen_invocation_args args = {.named = {{"THIS", value}}};
            co_return co_await co_gen_call_functum(bidx, rightarrow, args);
        }

        void kill_entry_value(block_index bidx, value_index vidx)
        {
            assert(this->state.blocks.at(bidx).instructions.empty());
            this->state.blocks.at(bidx).entry_state.erase(get_local_index(vidx));
            this->state.blocks.at(bidx).current_state.erase(get_local_index(vidx));
        }
        auto co_generate_logic_and(block_index& bidx, expression_binary input) -> typename CoroutineProvider::template co_type< value_index >
        {
            assert(bidx == block_index(0) || this->state.blocks.at(0).terminator.has_value());
            auto result_bool = this->create_local_value(bool_type{});
            auto lhs_block = this->generate_subblock(bidx, "logic_and_lhs");
            this->generate_jump(bidx, lhs_block);
            auto false_block = this->generate_subblock(bidx, "logic_and_false");
            auto true_block = this->generate_subblock(bidx, "logic_and_true");
            auto after_block = this->generate_subblock(bidx, "logic_and_after");
            auto lhs = co_await co_generate_bool_expr(lhs_block, input.lhs);
            auto rhs_block = this->generate_subblock(lhs_block, "logic_and_rhs");
            this->generate_branch(lhs, lhs_block, rhs_block, false_block);
            this->kill_entry_value(rhs_block, lhs);
            auto rhs = co_await co_generate_bool_expr(rhs_block, input.rhs);
            this->generate_branch(rhs, rhs_block, true_block, false_block);
            vmir2::load_const_bool set_false;
            set_false.value = false;
            set_false.target = get_local_index(result_bool);
            this->emit(false_block, set_false);
            vmir2::load_const_bool set_true;
            set_true.value = true;
            set_true.target = get_local_index(result_bool);
            this->emit(true_block, set_true);
            this->generate_jump(false_block, after_block);
            this->generate_jump(true_block, after_block);
            this->generate_survivor_local(false_block, after_block, get_local_index(result_bool));
            bidx = after_block;
            assert(bidx == block_index(0) || this->state.blocks.at(0).terminator.has_value());
            co_return result_bool;
        }
        auto co_generate_logic_nand(block_index& bidx, expression_binary input) -> typename CoroutineProvider::template co_type< value_index >
        {
            assert(bidx == block_index(0) || this->state.blocks.at(0).terminator.has_value());
            auto result_bool = this->create_local_value(bool_type{});
            auto lhs_block = this->generate_subblock(bidx, "logic_nand_lhs");
            this->generate_jump(bidx, lhs_block);
            auto false_block = this->generate_subblock(bidx, "logic_nand_false");
            auto true_block = this->generate_subblock(bidx, "logic_nand_true");
            auto after_block = this->generate_subblock(bidx, "logic_nand_after");
            auto lhs = co_await co_generate_bool_expr(lhs_block, input.lhs);
            auto rhs_block = this->generate_subblock(lhs_block, "logic_nand_rhs");
            this->generate_branch(lhs, lhs_block, rhs_block, true_block);
            this->kill_entry_value(rhs_block, lhs);
            auto rhs = co_await co_generate_bool_expr(rhs_block, input.rhs);
            this->generate_branch(rhs, rhs_block, false_block, true_block);
            vmir2::load_const_bool set_false;
            set_false.value = false;
            set_false.target = get_local_index(result_bool);
            this->emit(false_block, set_false);
            vmir2::load_const_bool set_true;
            set_true.value = true;
            set_true.target = get_local_index(result_bool);
            this->emit(true_block, set_true);
            this->generate_jump(false_block, after_block);
            this->generate_jump(true_block, after_block);
            this->generate_survivor_local(false_block, after_block, get_local_index(result_bool));
            bidx = after_block;
            assert(bidx == block_index(0) || this->state.blocks.at(0).terminator.has_value());
            co_return result_bool;
        }

        auto co_generate(block_index& bidx, expression_binary input) -> typename CoroutineProvider::template co_type< value_index >
        {
            if (logic_operators.contains(input.operator_str))
            {
                if (input.operator_str == "&&")
                {
                    co_return co_await co_generate_logic_and(bidx, input);
                }

                if (input.operator_str == "!&")
                {
                    co_return co_await co_generate_logic_nand(bidx, input);
                }
            }
            auto lhs = co_await co_generate_expr(bidx, input.lhs);
            auto rhs = co_await co_generate_expr(bidx, input.rhs);

            type_symbol lhs_type = this->current_type(bidx, lhs);
            type_symbol rhs_type = this->current_type(bidx, rhs);

            type_symbol lhs_underlying_type = remove_ref(lhs_type);
            type_symbol rhs_underlying_type = remove_ref(rhs_type);

            type_symbol lhs_function = submember{lhs_underlying_type, "OPERATOR" + input.operator_str};
            type_symbol rhs_function = submember{rhs_underlying_type, "OPERATOR" + input.operator_str + "RHS"};
            invotype lhs_param_info{.named = {{"THIS", lhs_type}, {"OTHER", rhs_type}}};
            invotype rhs_param_info{.named = {{"THIS", rhs_type}, {"OTHER", lhs_type}}};

            auto lhs_exists_and_callable_with = co_await prv.instanciation(initialization_reference{.initializee = lhs_function, .parameters = lhs_param_info});

            if (lhs_exists_and_callable_with)
            {
                auto lhs_args = codegen_invocation_args{.named = {{"THIS", lhs}, {"OTHER", rhs}}};
                co_return co_await co_gen_call_functum(bidx, lhs_function, lhs_args);
            }

            auto rhs_exists_and_callable_with = co_await prv.instanciation(initialization_reference{.initializee = rhs_function, .parameters = rhs_param_info});

            if (rhs_exists_and_callable_with)
            {
                auto rhs_args = codegen_invocation_args{.named = {{"THIS", rhs}, {"OTHER", lhs}}};
                co_return co_await co_gen_call_functum(bidx, rhs_function, rhs_args);
            }

            throw std::logic_error("Found neither " + to_string(lhs_function) + " callable with (" + to_string(lhs_type) + ", " + to_string(rhs_type) + ") nor " + to_string(rhs_function) + " callable with (" + to_string(rhs_type) + ", " + to_string(lhs_type) + ")");
        }

        auto co_generate(block_index& bidx, expression_numeric_literal input) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto val = this->create_numeric_literal(input.value);
            assert(val != 0);
            auto val_type = this->current_type(bidx, val);
            std::cout << "Generated numeric literal " << val << " of type " << to_string(val_type) << std::endl;
            co_return val;
        }

        auto co_generate(block_index& bidx, expression_unary_postfix input) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto val = co_await co_generate_expr(bidx, input.lhs);
            auto oper = this->get_class_member(bidx, val, "OPERATOR" + input.operator_str);
            co_return co_await co_gen_call_functum(bidx, oper, codegen_invocation_args{.named = {{"THIS", val}}});
        }

        auto co_generate(block_index& bidx, expression_unary_prefix input) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto val = co_await co_generate_expr(bidx, input.rhs);
            auto oper = this->get_class_member(bidx, val, "OPERATOR" + input.operator_str + "PREFIX");
            co_return co_await co_gen_call_functum(bidx, oper, codegen_invocation_args{.named = {{"THIS", val}}});
        }

        auto co_generate(block_index& bidx, expression_multibind const& what) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto lhs_val = co_await co_generate_expr(bidx, what.lhs);

            codegen_invocation_args invoke_brackets_args;
            invoke_brackets_args.named["THIS"] = lhs_val;

            for (auto& arg : what.bracketed)
            {
                invoke_brackets_args.positional.push_back(co_await co_generate_expr(bidx, arg));
            }

            type_symbol lhs_class_type = this->current_type(bidx, lhs_val);
            lhs_class_type = remove_ref(lhs_class_type);
            auto call_brackets_operator = submember{lhs_class_type, what.operator_str};

            co_return co_await co_gen_call_functum(bidx, call_brackets_operator, invoke_brackets_args);
        }

        auto get_class_member(block_index bidx, value_index val, std::string func)
        {
            auto val_type = this->current_type(bidx, val);
            auto val_class = remove_ref(val_type);
            auto func_ref = submember{val_class, func};
            return func_ref;
        }

        auto co_generate_bool_expr(block_index& bidx, expression expr) -> typename CoroutineProvider::template co_type< value_index >
        {
            return this->co_generate_typed_expr(bidx, expr, bool_type{});
        }

        auto co_generate_typed_expr(block_index& bidx, expression expr, type_symbol target_type) -> typename CoroutineProvider::template co_type< value_index >
        {
            std::string expr_str = quxlang::to_string(expr);
            auto expr_val = co_await co_generate_expr(bidx, expr);
            assert(bidx == block_index(0) || this->state.blocks.at(0).terminator.has_value());

            auto type_of_expr = this->current_type(bidx, expr_val);

            if (type_of_expr == target_type)
            {
                co_return expr_val;
            }

            implicitly_convertible_to_query query;
            query.from = type_of_expr;
            query.to = target_type;

            bool convertible = co_await prv.implicitly_convertible_to(query);

            if (!convertible)
            {
                throw std::logic_error("Cannot convert " + quxlang::to_string(type_of_expr) + " to " + quxlang::to_string(target_type));
            }

            co_return co_await co_gen_implicit_conversion(bidx, expr_val, target_type);
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_if_statement const& st) -> typename CoroutineProvider::template co_type< void >
        {
            block_index after_block = this->generate_subblock(current_block, "if_statement_after");
            block_index condition_block = this->generate_subblock(current_block, "if_statement_condition");
            block_index if_block = this->generate_subblock(current_block, "if_block");

            this->generate_jump(current_block, condition_block);

            auto cond = co_await co_generate_bool_expr(condition_block, st.condition);

            this->generate_branch(cond, condition_block, if_block, after_block);

            co_await co_generate_function_block(if_block, st.then_block, "if_then");
            this->generate_jump(if_block, after_block);

            if (st.else_block.has_value())
            {
                block_index else_block = this->generate_subblock(current_block, "if_statement_else");
                co_await co_generate_function_block(else_block, *st.else_block, "if_else");
                this->generate_jump(else_block, after_block);
            }

            current_block = after_block;

            co_return;
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_while_statement const& st) -> typename CoroutineProvider::template co_type< void >
        {
            block_index condition_block = this->generate_subblock(current_block, "while_condition");
            block_index body_block = this->generate_subblock(current_block, "while_body");
            block_index after_block = this->generate_subblock(current_block, "while_after");

            this->generate_jump(current_block, condition_block);

            auto cond = co_await co_generate_bool_expr(condition_block, st.condition);

            this->generate_branch(cond, condition_block, body_block, after_block);
            co_await co_generate_function_block(body_block, st.loop_block, "while_statement");
            this->generate_jump(body_block, condition_block);

            current_block = after_block;

            co_return;
        }

        auto generate_branch(value_index condition, block_index from, block_index true_branch, block_index false_branch) -> void
        {
            if (this->state.blocks.at(from).terminator.has_value())
            {
                throw std::logic_error("Cannot branch from a block that already has a terminator");
            }
            this->state.blocks.at(from).terminator = vmir2::branch{.condition = get_local_index(condition), .target_true = block_index(true_branch), .target_false = block_index(false_branch)};
        }

        auto block(block_index blk) -> codegen_block&
        {
            return this->state.blocks.at(blk);
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_expression_statement const& st) -> typename CoroutineProvider::template co_type< void >
        {
            QUXLANG_DEBUG_VALUE(quxlang::to_string(st.expr));

            QUXLANG_COMPILER_BUG_IF(this->has_terminator(current_block), "Expected no terminator in current block");

            block_index expr_block = this->generate_subblock(current_block, "expr_statement");
            block_index after_block = this->generate_subblock(current_block, "expr_after");

            this->generate_jump(current_block, expr_block);
            co_await co_generate_void_expr(expr_block, st.expr);
            this->generate_jump(expr_block, after_block);

            current_block = after_block;

            co_return;
        }

        [[nodiscard]] auto co_generate_void_expr(block_index& bidx, expression const& expr) -> typename CoroutineProvider::template co_type< value_index >
        {
            return co_generate_expr(bidx, expr);
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_block const& st) -> typename CoroutineProvider::template co_type< void >
        {
            co_await co_generate_function_block(current_block, st, "function_block");
            co_return;
        }

        auto generate_variable_local(block_index& current_block, std::string name, type_symbol var_type) -> value_index
        {
            auto idx = this->create_local_value(var_type);
            this->state.blocks.at(current_block).lookup_values[name] = idx;
            return idx;
        }

        auto generate_survivor_local(block_index from, block_index to, local_index survivor) -> void
        {
            this->block(to).entry_state[survivor] = this->block(from).current_state[survivor];
            this->block(to).current_state[survivor] = this->block(from).current_state[survivor];

            assert(this->block(to).instructions.empty());
        }

        auto generate_survivor_lookup(block_index from, block_index to, std::string name) -> void
        {
            this->block(to).lookup_values[name] = this->block(from).lookup_values.at(name);
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_var_statement const& st) -> typename CoroutineProvider::template co_type< void >
        {
            std::string type_str = quxlang::to_string(st.type);
            std::string context_str = quxlang::to_string(ctx);
            type_symbol var_type = (co_await prv.lookup({.context = ctx, .type = st.type})).value();

            block_index new_expr_block = this->generate_subblock(current_block, "var_new");
            block_index after_block = this->generate_subblock(current_block, "var_after");
            block_index initial_block = current_block;

            auto idx = this->generate_variable_local(new_expr_block, st.name, var_type);

            std::string var_type_name = quxlang::to_string(var_type);

            codegen_invocation_args args;

            args.named["THIS"] = idx;

            // Generate new blocks for after an intialization steps.

            this->generate_jump(current_block, new_expr_block);
            current_block = new_expr_block;

            // TODO: Function var statement needs named constructor support
            for (auto const& init : st.initializers)
            {
                auto init_idx = co_await co_generate_expr(new_expr_block, init);
                args.positional.push_back(init_idx);
            }

            auto ctor = submember{.of = var_type, .name = "CONSTRUCTOR"};
            co_await this->co_gen_call_functum(new_expr_block, ctor, args);

            auto class_default_dtor = co_await prv.class_default_dtor(var_type);
            if (class_default_dtor)
            {
                if (!state.non_trivial_dtors.contains(var_type))
                {
                    state.non_trivial_dtors[var_type] = class_default_dtor.value();
                }

                // TODO: Consider re-adding this for non-default dtors later.
                // co_await emitter.gen_defer_dtor(idx, dtor.value(), codegen_invocation_args{.named = {{"THIS", idx}}});
            }

            vmir2::slot_state new_state = state.blocks.at(new_expr_block).current_state[get_local_index(idx)];
            assert(new_state.valid());

            this->generate_jump(new_expr_block, after_block);
            this->generate_survivor_local(new_expr_block, after_block, get_local_index(idx));
            this->generate_survivor_lookup(new_expr_block, after_block, st.name);
            current_block = after_block;

            // the after_block is cloned from the parent block, so the new variable isn't alive in that block
            // We want all the temporaries to be destroyed so we cloned the parent block twice, and the after
            // block is the parent + the new variable, which won't contain the temporary objects generated above.

            co_return;
        }

        auto co_generate(block_index& bidx, expression_thisdot_reference what) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto this_reference = freebound_identifier{"THIS"};
            auto value = co_await this->co_lookup_symbol(bidx, this_reference);
            if (!value)
            {
                throw std::logic_error("Cannot find " + to_string(this_reference));
            }
            auto field = co_await this->co_generate_dot_access(bidx, *value, what.field_name);
            co_return field;
        }

        auto co_generate(block_index& bidx, expression_string_literal expr) -> typename CoroutineProvider::template co_type< value_index >
        {
            throw rpnx::unimplemented();
        }

        auto co_generate(block_index& bidx, expression_dotreference what) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto parent = co_await co_generate_expr(bidx, what.lhs);
            co_return co_await co_generate_dot_access(bidx, parent, what.field_name);
        }

        auto co_generate_dot_access(block_index& bidx, value_index base, std::string field_name) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto base_type = this->current_type(bidx, base);
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
                    access.base_index = get_local_index(base);
                    access.field_name = field.name;
                    type_symbol result_ref_type = recast_reference(base_type.template get_as< pointer_type >(), field.type);
                    auto result_idx = create_local_value(result_ref_type);
                    access.store_index = get_local_index(result_idx);
                    //std::cout << "Created field access " << access.store_index << " for " << field_name << " in " << to_string(base_type) << std::endl;

                    this->emit(bidx, access);
                    co_return result_idx;
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

        auto co_generate_builtin_ctor(instanciation_reference const& func) -> typename CoroutineProvider::template co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!qualified_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();
            block_index current_block = block_index(0);
            co_await co_generate_ctor_delegates(current_block, func, {});
            co_await co_generate_builtin_return(current_block);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_copy_ctor(instanciation_reference const& func) -> typename CoroutineProvider::template co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!qualified_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();
            block_index current_block = block_index(0);
            co_await co_generate_copy_ctor_delegates(current_block, func);
            co_await co_generate_builtin_return(current_block);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_move_ctor(instanciation_reference const& func) -> typename CoroutineProvider::template co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!qualified_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();
            block_index current_block = block_index(0);
            co_await co_generate_move_ctor_delegates(current_block, func);
            co_await co_generate_builtin_return(current_block);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_dtor(instanciation_reference const& func) -> typename CoroutineProvider::template co_type< quxlang::vmir2::functanoid_routine3 >
        {

            co_await co_generate_arg_info(func);
            throw rpnx::unimplemented();
            co_await co_generate_dtors();
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_return(block_index bidx) -> typename CoroutineProvider::template co_type< void >
        {
            // TODO: Implement implied returns
            this->generate_return(bidx);
            co_return;
        }
        auto get_result()
        {

            vmir2::functanoid_routine3 result;
            for (auto const& [type, dtor] : this->state.non_trivial_dtors)
            {
                result.non_trivial_dtors[type] = dtor;
            }
            for (block_index i = block_index(0); i < this->state.blocks.size(); i++)
            {
                codegen_block& block = this->state.blocks.at(i);
                vmir2::executable_block block2;

                block2.instructions = MOVEREL(block.instructions);
                block2.entry_state = block.entry_state;
                block2.terminator = MOVEREL(block.terminator);
                block2.dbg_name = block.dbg_name;

                result.blocks.push_back(block2);
            }
            result.local_types = state.locals;
            result.parameters = state.params;

            return result;
        }

        auto co_generate_body(block_index& current_block, instanciation_reference const& func) -> typename CoroutineProvider::template co_type< void >
        {
            auto const& inst = func;

            auto& function_ref = inst.temploid;

            auto function_decl_opt = co_await this->prv.function_declaration(function_ref);
            assert(function_decl_opt.has_value());
            ast2_function_declaration& function_decl = function_decl_opt.value();

            co_await co_generate_function_block(current_block, function_decl.definition.body, "body");

            if (this->state.blocks.at(current_block).terminator.has_value() == false)
            {
                // TODO: Check if default return is allowed.
                this->generate_return(current_block);
            }

            co_return;
        }

        void generate_return(block_index idx)
        {
            this->state.blocks[idx].terminator = vmir2::ret();
        }

        auto co_generate_dtor_references() -> typename CoroutineProvider::template co_type< void >
        {
            // Loop through all local slots and check if they have non-trivial dtors, then add
            // dtor references to non_trivial_dtors if they do.
            for (codegen_value const& genvalue : this->state.genvalues)
            {
                if (!genvalue.template type_is< codegen_local >())
                {
                    continue;
                }
                auto& slot = state.locals.at(genvalue.template get_as< codegen_local >().local_index);
                auto dtor = co_await prv.class_default_dtor(slot.type);
                if (dtor)
                {
                    assert(!this->state.non_trivial_dtors.contains(slot.type) || this->state.non_trivial_dtors[slot.type] == *dtor);
                    this->state.non_trivial_dtors[slot.type] = *dtor;
                }
            }

            co_return;
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_return_statement const& st) -> typename CoroutineProvider::template co_type< void >
        {
            auto return_arg_opt = this->local_value_direct_lookup(current_block, "RETURN");

            if (return_arg_opt.has_value())
            {
                auto return_arg = return_arg_opt.value();

                if (st.expr.has_value())
                {
                    auto expr_index = co_await co_generate_expr(current_block, st.expr.value());

                    codegen_invocation_args args;
                    args.named["THIS"] = return_arg;
                    args.named["OTHER"] = expr_index;

                    auto return_type = current_type(current_block, return_arg);
                    if (!typeis< nvalue_slot >(return_type))
                    {
                        throw std::logic_error("RETURN parameter has the wrong type");
                    }
                    return_type = type_symbol(as< nvalue_slot >(return_type).target);
                    auto ctor = submember{.of = return_type, .name = "CONSTRUCTOR"};
                    co_await co_gen_call_functum(current_block, ctor, args);
                    this->generate_return(current_block);
                    co_return;
                }

                co_await co_generate_builtin_return(current_block);
            }
            else
            {
                // The only situation where ctx cannot be an instanciation_reference is the constexpr void evaluation
                // which will not have any return type.
                auto return_type = co_await prv.functanoid_return_type(ctx.get_as< instanciation_reference >());
                assert(typeis< void_type >(return_type));
                this->generate_return(current_block);
            }

            co_return;
        }

        [[nodiscard]] auto co_generate_fblock_statement(block_index& current_block, function_statement const& st) -> typename CoroutineProvider::template co_type< void >
        {
            co_await rpnx::apply_visitor< typename CoroutineProvider::template co_type< void > >(
                [&](auto st) -> typename CoroutineProvider::template co_type< void >
                {
                    co_return co_await this->co_generate_statement_ovl(current_block, st);
                },
                st);
            co_return;
        }

        auto generate_subblock(block_index& current_block, std::string block_from) -> block_index
        {
            this->state.blocks.emplace_back();

            codegen_block& new_block = this->state.blocks.back();
            new_block.dbg_name = block_from;
            codegen_block& current_block_ref = this->state.blocks.at(current_block);

            new_block.entry_state = current_block_ref.current_state;
            new_block.current_state = new_block.entry_state;
            new_block.lookup_values = current_block_ref.lookup_values;

            return block_index(this->state.blocks.size() - 1);
        }

        [[nodiscard]] auto co_generate_function_block(block_index& current_block, function_block const& block, std::string block_from) -> typename CoroutineProvider::template co_type< void >
        {
            assert(!this->state.blocks.at(current_block).terminator.has_value());
            auto new_block = this->generate_subblock(current_block, block_from + "_block_new");

            assert(!this->state.blocks.at(new_block).terminator.has_value());

            auto after_block = this->generate_subblock(current_block, block_from + "_block_after");
            assert(!this->state.blocks.at(after_block).terminator.has_value());

            this->generate_jump(current_block, new_block);

            for (auto const& statement : block.statements)
            {
                assert(!this->state.blocks.at(new_block).terminator.has_value());
                co_await co_generate_fblock_statement(new_block, statement);
            }

            if (!this->state.blocks.at(new_block).terminator.has_value())
            {
                this->generate_jump(new_block, after_block);
            }

            assert(this->state.blocks.at(current_block).terminator.has_value());
            current_block = after_block;
            assert(!this->state.blocks.at(after_block).terminator.has_value());
            co_return;
        }

        void generate_jump(block_index& from, block_index& to)
        {
            auto& from_block = this->state.blocks.at(from);
            if (from_block.terminator.has_value())
            {
                throw std::logic_error("Cannot jump from a block that already has a terminator");
            }

            vmir2::jump jump_instruction{.target = to};
            from_block.terminator = jump_instruction;
        }

        bool has_terminator(block_index const& block) const
        {
            return this->state.blocks.at(block).terminator.has_value();
        }

        auto co_generate_arg_info(instanciation_reference func) -> typename CoroutineProvider::template co_type< void >
        {
            QUXLANG_DEBUG_VALUE(quxlang::to_string(func));
            // Precondition: Func is a fully instanciated symbol

            assert(!qualified_is_contextual(func));
            instanciation_reference inst = func;

            // This function should be called before generating any blocks.
            assert(this->state.blocks.empty());

            // TODO: Support AUTO return types
            auto sig = co_await prv.functanoid_sigtype(inst);

            type_symbol return_type = sig.return_type.value_or(void_type()); // co_await prv.functanoid_return_type(inst);

            if (!typeis< void_type >(return_type))
            {
                type_symbol return_parameter_type = create_nslot(return_type);
                type_symbol const& return_local_type = return_type;
                this->state.params.named["RETURN"].type = return_parameter_type;
                auto return_valueidx = this->create_local_value(return_local_type);
                this->state.params.named["RETURN"].local_index = get_local_index(return_valueidx);
                this->state.top_level_lookups["RETURN"] = return_valueidx;
            }

            auto arg_names = co_await prv.function_param_names(inst.temploid);

            std::size_t positional_index = 0;
            for (auto const& param_name : arg_names.positional)
            {
                type_symbol const& param_type = inst.params.positional.at(positional_index);
                if (param_name.has_value())
                {
                    auto param_idx = this->create_local_value(param_type);
                    this->state.params.positional.push_back(vmir2::routine_parameter{.type = param_type, .local_index = get_local_index(param_idx)});
                    this->state.top_level_lookups[*param_name] = param_idx;
                    positional_index++;
                }
            }
            for (auto const& [api_name, param_type] : inst.params.named)
            {
                std::optional< std::string > arg_name;
                if (arg_names.named.contains(api_name))
                {
                    arg_name = arg_names.named.at(api_name);
                }
                auto local_type = param_type;
                if (typeis< nvalue_slot >(local_type))
                {
                    local_type = type_symbol(as< nvalue_slot >(local_type).target);
                }
                else if (typeis< dvalue_slot >(local_type))
                {
                    local_type = as< dvalue_slot >(local_type).target;
                }
                auto arg_idx = this->create_local_value(local_type);
                this->state.params.named[api_name] = {
                    .type = param_type,
                    .local_index = get_local_index(arg_idx),
                };

                // If a local name is provided, it's strongly defined and the API name is weakly defined.
                // Otherwise, the API name is strongly defined.

                // Weakly defined names can be shadowed by strongly defined names.

                // TODO: Check for conflicts with existing names in the top-level lookups.

                if (arg_name.has_value())
                {
                    this->state.top_level_lookups[arg_name.value()] = arg_idx;
                    this->state.top_level_lookups_weak[api_name] = arg_idx;
                }
                else
                {
                    this->state.top_level_lookups[api_name] = arg_idx;
                }
            }

            co_return;
        }

        void generate_entry_block()
        {
            // This function should be called before generating any blocks.
            assert(this->state.blocks.empty());
            this->state.blocks.emplace_back();
            codegen_block& entry_block = this->state.blocks.back();
            auto& entry_state = entry_block.entry_state;
            vmir2::codegen_state_engine(entry_state, this->state.locals, this->state.params).apply_entry();
            entry_block.current_state = entry_state;
        }

        [[nodiscard]] auto co_generate_functanoid(instanciation_reference func) -> typename CoroutineProvider::template co_type< vmir2::functanoid_routine3 >
        {
            assert(!qualified_is_contextual(func));
            co_await this->co_generate_arg_info(func);
            this->generate_entry_block();
            block_index current_block(0);
            if (!co_await prv.function_builtin(func.temploid))
            {
                if (typeis< submember >(func.temploid.templexoid) && func.temploid.templexoid.template get_as< submember >().name == "CONSTRUCTOR")
                {
                    co_await co_generate_ctor_delegates(current_block, func);
                }

                co_await co_generate_body(current_block, func);
            }
            co_await co_generate_dtors();

            co_return get_result();
        }

        auto add_nontrivial_default_dtor(type_symbol const& type, type_symbol const& dtor) -> void
        {
            state.non_trivial_dtors[type] = dtor;
        }

        auto co_generate_dtors() -> typename CoroutineProvider::template co_type< void >
        {
            // Loop through all local slots and check if they have non-trivial dtors, then add
            // dtor references to non_trivial_dtors if they do.
            for (auto const& slot : this->state.locals)
            {
                auto dtor = co_await prv.class_default_dtor(slot.type);
                if (dtor.has_value())
                {
                    assert(!this->state.non_trivial_dtors.contains(slot.type) || this->state.non_trivial_dtors[slot.type] == *dtor);
                    this->state.non_trivial_dtors[slot.type] = *dtor;
                }
            }

            co_return;
        }

        [[nodiscard]] auto co_generate_ctor_delegates(block_index& bidx, instanciation_reference const& func) -> typename CoroutineProvider::template co_type< void >
        {
            temploid_reference const& function = func.temploid;

            std::optional< ast2_function_declaration > const& function_ast = co_await prv.function_declaration(function);

            QUXLANG_COMPILER_BUG_IF(!function_ast.has_value(), "Expected function declaration to be defined");

            auto const& decl = function_ast.value();

            std::vector< delegate > delegates;

            for (auto& dlg : decl.definition.delegates)
            {
                // TODO: support complex types

                if (!dlg.target.type_is< submember >() || !dlg.target.get_as< submember >().of.type_is< context_reference >())
                {
                    throw rpnx::unimplemented();
                }

                delegate dlg2;
                dlg2.name = dlg.target.get_as< submember >().name;

                // TODO: Support named arguments in delegates
                for (expression const& arg : dlg.args)
                {
                    dlg2.args.push_back(expression_arg{.value = arg});
                }

                delegates.push_back(dlg2);
            }

            co_await co_generate_ctor_delegates(bidx, func, delegates);

            co_return;
        }

        auto get_invocation_args(codegen_invocation_args const& args) -> vmir2::invocation_args
        {
            vmir2::invocation_args result;
            for (auto const& [name, value] : args.named)
            {
                result.named[name] = get_local_index(value);
            }
            for (auto const& value : args.positional)
            {
                result.positional.push_back(get_local_index(value));
            }
            return result;
        }

        auto co_copy_ref(block_index& current_block, value_index val) -> typename CoroutineProvider::template co_type< value_index >
        {
            type_symbol val_type = this->current_type(current_block, val);
            // This function should convert an mref to tref

            if (!val_type.type_is< pointer_type >())
            {
                throw std::logic_error("Expected a reference type");
            }

            auto vptr = val_type.get_as< pointer_type >();

            if (vptr.ptr_class != pointer_class::ref)
            {
                throw std::logic_error("Expected a reference type");
            }

            auto copy_idx = this->create_local_value(vptr);
            this->emit(current_block, vmir2::copy_reference{.from_index = get_local_index(val), .to_index = get_local_index(copy_idx)});
            co_return copy_idx;
        }

        auto co_generate_copy_ctor_delegates(block_index& current_block, instanciation_reference const& func) -> typename CoroutineProvider::template co_type< void >
        {
            instanciation_reference const& inst = func;
            auto const& sel = inst.temploid;

            auto functum = sel.templexoid;

            type_symbol cls;

            QUXLANG_COMPILER_BUG_IF(!typeis< submember >(functum), "Expected constructor to be submember");

            cls = as< submember >(functum).of;

            auto const& fields = co_await prv.class_field_list(cls);

            // Implicit copy ctor delegates just calls copy constructor on all fields.

            codegen_invocation_args fields_args;

            for (class_field const& fld : fields)
            {
                auto fslot = this->create_local_value(fld.type);
                fields_args.named[fld.name] = fslot;
            }

            auto thisidx = this->local_value_direct_lookup(current_block, "THIS");

            QUXLANG_COMPILER_BUG_IF(!thisidx.has_value(), "Expected THIS to be defined");

            auto thisidx_value = thisidx.value();

            auto otheridx = this->local_value_direct_lookup(current_block, "OTHER");
            QUXLANG_COMPILER_BUG_IF(!otheridx.has_value(), "Expected OTHER to be defined");
            auto otheridx_value = otheridx.value();

            this->emit(current_block, vmir2::struct_delegate_new{.on_value = get_local_index(thisidx_value), .fields = get_invocation_args(fields_args)});

            for (class_field const& fld : fields)
            {
                auto temporary_block = this->generate_subblock(current_block, "copy_ctor_temp_" + fld.name);
                auto after_ctor_block = this->generate_subblock(current_block, "copy_ctor_after_" + fld.name);
                this->generate_jump(current_block, temporary_block);
                auto other_idx_copy = co_await this->co_copy_ref(temporary_block, otheridx_value);
                auto other_field = co_await this->co_generate_dot_access(temporary_block, other_idx_copy, fld.name);
                auto field_type = fld.type;
                assert(!qualified_is_contextual(field_type));
                auto field_copy_ctor_functum = submember{.of = field_type, .name = "CONSTRUCTOR"};
                codegen_invocation_args args;
                args.named["THIS"] = fields_args.named.at(fld.name);
                args.named["OTHER"] = other_field;
                co_await this->co_gen_call_functum(temporary_block, field_copy_ctor_functum, args);
                this->generate_jump(temporary_block, after_ctor_block);
                this->generate_survivor_local(temporary_block, after_ctor_block, get_local_index(fields_args.named.at(fld.name)));
                current_block = after_ctor_block;
            }

            // TODO: Include SCN in delegate constructors?
            // this->emit(current_block, vmir2::struct_complete_new{.on_value = get_local_index(thisidx_value)});
        }

        auto co_generate_move(block_index& current_block, value_index val) -> typename CoroutineProvider::template co_type< value_index >
        {
            type_symbol val_type = this->current_type(current_block, val);
            // This function should convert an mref to tref

            if (!val_type.type_is< pointer_type >())
            {
                // No-op if the value is not a reference type
                co_return val;
            }

            auto vptr = val_type.get_as< pointer_type >();

            if (vptr.ptr_class != pointer_class::ref)
            {
                // This is another non-reference type, so we can just return the value as is.
                co_return val;
            }

            if (vptr.qual == qualifier::mut)
            {
                auto tref_type = vptr;
                vptr.qual = qualifier::temp;
                auto tref = this->create_local_value(tref_type);
                this->emit(current_block, vmir2::cast_reference{.source_ref_index = this->get_local_index(val), .target_ref_index = this->get_local_index(tref)});
                co_return tref;
            }

            // TODO: Maybe this should be an error if e.g. it's a const ref?

            co_return val;
        }

        auto co_generate_move_ctor_delegates(block_index& current_block, instanciation_reference const& func) -> typename CoroutineProvider::template co_type< void >
        {

            instanciation_reference const& inst = func;
            auto const& sel = inst.temploid;

            auto functum = sel.templexoid;

            type_symbol cls;

            QUXLANG_COMPILER_BUG_IF(!typeis< submember >(functum), "Expected constructor to be submember");

            cls = as< submember >(functum).of;

            // This function is for default ctors, it should just default construct all member variables.

            auto const& fields = co_await prv.class_field_list(cls);

            codegen_invocation_args fields_args;

            for (class_field const& fld : fields)
            {
                auto fslot = this->create_local_value(fld.type);
                fields_args.named[fld.name] = fslot;
            }

            auto thisidx = this->local_value_direct_lookup(current_block, "THIS");

            QUXLANG_COMPILER_BUG_IF(!thisidx.has_value(), "Expected THIS to be defined");

            auto thisidx_value = thisidx.value();

            auto otheridx = this->local_value_direct_lookup(current_block, "OTHER");
            QUXLANG_COMPILER_BUG_IF(!otheridx.has_value(), "Expected OTHER to be defined");
            auto otheridx_value = otheridx.value();

            for (class_field const& fld : fields)
            {
                auto temporary_block = this->generate_subblock(current_block, "move_ctor_temp_" + fld.name);
                auto after_ctor_block = this->generate_subblock(current_block, "move_ctor_after_" + fld.name);
                this->generate_jump(current_block, temporary_block);
                auto other_idx_copy = co_await this->co_copy_ref(temporary_block, otheridx_value);
                auto other_field = co_await this->co_generate_dot_access(temporary_block, other_idx_copy, fld.name);
                auto field_type = fld.type;
                assert(!qualified_is_contextual(field_type));
                auto field_ctor_functum = submember{.of = field_type, .name = "CONSTRUCTOR"};
                other_field = co_await this->co_generate_move(temporary_block, other_field);
                codegen_invocation_args args;
                args.named["THIS"] = fields_args.named.at(fld.name);
                args.named["OTHER"] = other_field;
                co_await this->co_gen_call_functum(temporary_block, field_ctor_functum, args);
                this->generate_jump(temporary_block, after_ctor_block);
                this->generate_survivor_local(temporary_block, after_ctor_block, get_local_index(fields_args.named.at(fld.name)));
                current_block = after_ctor_block;
            }
        }

        [[nodiscard]] auto co_generate_ctor_delegates(block_index& current_block, instanciation_reference const& func, std::vector< delegate > delegates) -> typename CoroutineProvider::template co_type< void >
        {

            instanciation_reference const& inst = func;
            auto const& sel = inst.temploid;

            auto functum = sel.templexoid;

            type_symbol cls;

            QUXLANG_COMPILER_BUG_IF(!typeis< submember >(functum), "Expected constructor to be submember");

            cls = as< submember >(functum).of;

            // This function is for default ctors, it should just default construct all member variables.

            auto const& fields = co_await prv.class_field_list(cls);

            codegen_invocation_args fields_args;

            for (class_field const& fld : fields)
            {
                auto fslot = this->create_local_value(fld.type);
                fields_args.named[fld.name] = fslot;
            }

            auto thisidx = this->local_value_direct_lookup(current_block, "THIS");

            QUXLANG_COMPILER_BUG_IF(!thisidx.has_value(), "Expected THIS to be defined");

            auto thisidx_value = thisidx.value();

            this->emit(current_block, vmir2::struct_delegate_new{.on_value = get_local_index(thisidx_value), .fields = get_invocation_args(fields_args)});

            std::set< std::string > found_delegate_names;
            for (delegate const& dlg : delegates)
            {
                found_delegate_names.insert(dlg.name);
            }

            // TODO: Drop temporaries between loop iterations
            for (class_field const& fld : fields)
            {
                if (!found_delegate_names.contains(fld.name))
                {
                    auto ctor = submember{.of = fld.type, .name = "CONSTRUCTOR"};
                    codegen_invocation_args args;
                    args.named["THIS"] = fields_args.named.at(fld.name);

                    co_await this->co_gen_call_functum(current_block, ctor, args);
                }
            }

            // TODO: Should this execute before the body?
            // this->emit(current_block, vmir2::struct_complete_new{.on_value = get_local_index(thisidx_value)});

            // frame.block(current_block).emit(vmir2::struct_complete_new{.on_value = thisidx_value});
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_assert_statement const& asrt) -> typename CoroutineProvider::template co_type< void >
        {
            block_index after_block = this->generate_subblock(current_block, "assert_statement_after");
            block_index condition_block = this->generate_subblock(current_block, "if_statement_condition");
            this->generate_jump(current_block, condition_block);
            value_index cond = co_await co_generate_bool_expr(condition_block, asrt.condition);
            vmir2::assert_instr asrt_instr{.condition = get_local_index(cond), .message = asrt.tagline.value_or("NO_MESSAGE_TAG"), .location = asrt.location};
            this->emit(condition_block, asrt_instr);
            this->generate_jump(condition_block, after_block);
            current_block = after_block;
            co_return;
        }
    };

} // namespace quxlang

#endif // RPNX_QUXLANG_CO_VMIR_EXPRESSION_EMITTER_HEADER
