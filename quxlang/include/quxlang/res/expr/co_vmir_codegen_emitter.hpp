// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_RES_EXPR_CO_VMIR_CODEGEN_EMITTER_HEADER_GUARD
#define QUXLANG_RES_EXPR_CO_VMIR_CODEGEN_EMITTER_HEADER_GUARD

#include "quxlang/ast2/ast2_entity.hpp"
#include "quxlang/bytemath.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/class_field_declaration.hpp"
#include "quxlang/data/class_layout.hpp"
#include "quxlang/data/compilation_result.hpp"
#include "quxlang/data/contextual_type_reference.hpp"
#include "quxlang/data/expression_call.hpp"
#include "quxlang/data/machine.hpp"
#include "quxlang/data/type_placement_info.hpp"
#include "quxlang/exception.hpp"
#include "quxlang/fixed_bytemath.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/res/implicitly_convertible_to.hpp"
#include "quxlang/res/symbol_type.hpp"
#include "quxlang/vmir2/assembler.hpp"
#include "quxlang/vmir2/state_engine.hpp"
#include "quxlang/vmir2/vmir2.hpp"
#include "rpnx/simple_coroutine.hpp"
#include "rpnx/uint64_base.hpp"
#include <quxlang/res/constexpr.hpp>

#include <assert.h>
#include <set>
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
        type_symbol attached_symbol;
        value_index bound_value;

        RPNX_MEMBER_METADATA(codegen_binding, attached_symbol, bound_value);
    };

    struct codegen_local
    {
        local_index local_index;

        RPNX_MEMBER_METADATA(codegen_local, local_index);
    };

    struct codegen_literal
    {
        type_symbol type;
        std::vector< std::byte > value;

        RPNX_MEMBER_METADATA(codegen_literal, type, value);
    };

    using codegen_value = rpnx::variant< codegen_binding, codegen_literal, codegen_local >;

    struct codegen_state
    {
        std::vector< codegen_value > genvalues{codegen_binding{.attached_symbol = void_type(), .bound_value = value_index(0)}};
        std::vector< vmir2::local_type > locals{vmir2::local_type{.type = void_type()}};
        std::vector< codegen_block > blocks;
        vmir2::routine_parameters params;
        std::map< type_symbol, type_symbol > non_trivial_dtors;
        std::map< std::string, value_index > codegen_numeric_literals;
        std::map< std::string, value_index > codegen_string_literals;
        std::map< std::string, value_index > top_level_lookups;
        std::map< std::string, value_index > top_level_lookups_weak;
        type_symbol context;
        std::optional< instanciation_reference > functanoid_type;

        std::map< std::string, rpnx::variant< constexpr_result, type_symbol > > scoped_definitions;
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

        auto set_scoped_definitions(std::map< std::string, rpnx::variant< constexpr_result, type_symbol > > defs) -> void
        {
            this->state.scoped_definitions = std::move(defs);
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
                return block.current_state.at(idx).alive();
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
                expr,
                [&](auto&& val)
                {
                    return co_generate(bidx, std::forward< decltype(val) >(val));
                });
            std::string expr_str = to_string(expr);
            assert(bidx == block_index(0) || this->state.blocks.at(0).terminator.has_value());
            co_return result;
        }

        auto co_gen_argument_adaptation(block_index& bidx, value_index val, type_symbol target_type, parameter_init_kind adaptation_kind) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto value_type = this->current_type(bidx, val);

            if (value_type == target_type)
            {
                assert(val != value_index(0));
                co_return val;
            }

            co_return co_await co_gen_construct_with_target_type(bidx, val, target_type, adaptation_kind);
        }

        auto nested_constructor_init_kind(parameter_init_kind init_method) -> parameter_init_kind
        {
            switch (init_method)
            {
            case parameter_init_kind::call:
                return parameter_init_kind::implicit_conversion;
            case parameter_init_kind::implicit_conversion:
            case parameter_init_kind::bind_only:
                return parameter_init_kind::bind_only;
            default:
                throw std::logic_error("Invalid constructor init kind");
            }
        }

        auto co_gen_construct_with_target_type(block_index& bidx, value_index source, type_symbol target_type, parameter_init_kind init_method) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto target_index = create_local_value(target_type);
            auto constructor_functum = submember{target_type, "CONSTRUCTOR"};
            auto constructor_init_kind = nested_constructor_init_kind(init_method);

            codegen_invocation_args ctor_args = {.named = {{"THIS", target_index}, {"OTHER", source}}};
            co_await co_gen_call_functum(bidx, constructor_functum, ctor_args, constructor_init_kind);

            co_return target_index;
        }

        auto co_gen_call_functum(block_index& bidx, type_symbol func, codegen_invocation_args args, parameter_init_kind init_method = parameter_init_kind::call) -> typename CoroutineProvider::template co_type< value_index >
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
                }
                calltype.positional.push_back(arg_type);
            }
            for (auto& [name, arg] : args.named)
            {
                auto arg_type = current_type(bidx, arg);
                bool is_alive = value_alive(bidx, arg);

                std::string name_copy = name;
                std::string arg_type_str = to_string(arg_type);

                // std::cout << " arg name=" << name << " index=" << arg << " is_alive=" << is_alive << " current_type=" << to_string(arg_type) << std::endl;
                if (!is_alive)
                {
                    assert(typeis< nvalue_slot >(arg_type));
                }

                calltype.named[name] = arg_type;
            }

            initialization_reference functanoid_unnormalized{.initializee = func, .parameters = calltype, .init_kind = init_method};

            // std::cout << "co_gen_call_functum initialization params: (" << quxlang::to_string(functanoid_unnormalized) << ")" << std::endl;
            //  Get call type

            auto kind = (co_await prv.symbol_type(func));
            if (kind != symbol_kind::functum)
            {
                auto func_str = to_string(func);

                if (kind == symbol_kind::noexist)
                {
                    compilation_error c;
                    c.structured_error = semantic_error{func_str + " does not exist"};

                    throw c;
                }
                else if (kind == symbol_kind::local_variable)
                {
                    throw std::logic_error("Error: cannot call local variable " + func_str + " as a functum");
                }
                else if (kind == symbol_kind::class_)
                {
                    throw std::logic_error("Error: cannot call class type " + func_str + " as a functum");
                }
                else
                {
                    throw std::logic_error("Error: symbol " + func_str + " is not a functum");
                }
            }
            auto instanciation = co_await prv.instanciation(functanoid_unnormalized);

            auto functum_overloeads = co_await prv.functum_overloads(func);

            for (auto const& overload : functum_overloeads)
            {
                std::cout << " - Candidate: " + to_string(overload.interface) << std::endl;
            }

            if (!instanciation)
            {
                std::string message = "Cannot call " + to_string(func) + " with " + quxlang::to_string(calltype);

                throw std::logic_error(message);
            }

            std::cout << "co_gen_call_functum selected instanciation: " << quxlang::to_string(*instanciation) << std::endl;

            co_return co_await this->co_gen_call_functanoid(bidx, instanciation.value(), args, init_method);
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
                auto attached_symbol = slot.template get_as< codegen_binding >().attached_symbol;
                assert(!type_is_contextual(attached_symbol));
                auto bound_type = this->current_type(bidx, bound_value);
                return attached_type_reference{.carrying_type = bound_type, .attached_symbol = attached_symbol};
            }

            auto local_idx = get_local_index(idx);
            auto& slot_state = block.current_state[local_idx];

            if (slot.template type_is< codegen_binding >())
            {
                auto& binding = slot.template get_as< codegen_binding >();
                return attached_type_reference{.carrying_type = current_type(bidx, binding.bound_value), .attached_symbol = binding.attached_symbol};
            }

            auto type = state.locals.at(slot.template get_as< codegen_local >().local_index).type;

            // NValue and DValue types appear only in the parameter types, the locals
            // are never nvalue or dvalue slots.
            assert(!type.template type_is< nvalue_slot >() && !type.template type_is< dvalue_slot >());

            if (slot_state.destroy_delegate)
            {
                type = dvalue_slot{.target = type};
            }
            else if (!slot_state.alive())
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

        auto resolve_functum_instanciation(block_index& bidx, type_symbol func, invotype calltype, parameter_init_kind init_method) -> typename CoroutineProvider::template co_type< instanciation_reference >
        {
            initialization_reference functanoid_unnormalized{.initializee = func, .parameters = calltype, .init_kind = init_method};

            auto kind = (co_await prv.symbol_type(func));
            if (kind != symbol_kind::functum)
            {
                throw std::logic_error("Expected functum symbol " + to_string(func));
            }

            auto instanciation = co_await prv.instanciation(functanoid_unnormalized);
            if (!instanciation)
            {
                throw std::logic_error("Cannot call " + to_string(func) + " with " + quxlang::to_string(calltype));
            }

            co_return *instanciation;
        }

        auto adapt_args_for_instanciation(block_index& bidx, instanciation_reference what, codegen_invocation_args expression_args, std::set< std::string > skip_named = {}) -> typename CoroutineProvider::template co_type< codegen_invocation_args >
        {
            codegen_invocation_args invocation_args;

            auto create_arg_value = [&](value_index arg_expr_index, type_symbol arg_target_type) -> typename CoroutineProvider::template co_type< value_index >
            {
                auto arg_expr_type = this->current_type(bidx, arg_expr_index);
                bool arg_alive = this->value_alive(bidx, arg_expr_index);

                if (arg_expr_type == arg_target_type)
                {
                    co_return arg_expr_index;
                }

                if (!arg_alive)
                {
                    assert(typeis< nvalue_slot >(arg_expr_type));
                }

                co_return co_await co_gen_argument_adaptation(bidx, arg_expr_index, arg_target_type, parameter_init_kind::call);
            };

            for (auto const& [name, arg_accepted_type] : what.params.named)
            {
                if (skip_named.contains(name))
                {
                    continue;
                }

                auto arg_expr_index = expression_args.named.at(name);
                invocation_args.named[name] = co_await create_arg_value(arg_expr_index, arg_accepted_type);
            }

            std::size_t positional_write = 0;
            for (std::size_t i = 0; i < what.params.positional.size(); i++)
            {
                auto arg_accepted_type = what.params.positional.at(i);
                auto arg_expr_index = expression_args.positional.at(positional_write++);
                invocation_args.positional.push_back(co_await create_arg_value(arg_expr_index, arg_accepted_type));
            }

            co_return invocation_args;
        }

        auto co_expect_storage_reference(block_index bidx, value_index storage_ref, bool require_mut, std::optional< type_symbol > projected_type = std::nullopt) -> typename CoroutineProvider::template co_type< ptrref_type >
        {
            auto storage_ref_type = this->current_type(bidx, storage_ref);
            if (!is_ref(storage_ref_type))
            {
                throw std::logic_error("Expected a storage reference");
            }

            auto ref_type = as< ptrref_type >(storage_ref_type);
            if (require_mut && ref_type.qual != qualifier::mut)
            {
                throw std::logic_error("Expected MUT& STORAGE for storage mutation");
            }

            auto storage_type = remove_ref(storage_ref_type);
            if (!typeis< storage >(storage_type) && !typeis< aligned_storage >(storage_type))
            {
                throw std::logic_error("Expected a storage-typed reference");
            }

            if (projected_type.has_value() && typeis< storage >(storage_type))
            {
                bool allowed = false;
                for (auto const& allowed_type : as< storage >(storage_type).storable_types)
                {
                    if (allowed_type == *projected_type)
                    {
                        allowed = true;
                        break;
                    }
                }
                if (!allowed)
                {
                    throw std::logic_error("Projected type is not permitted by storage type");
                }
            }
            else if (projected_type.has_value() && typeis< aligned_storage >(storage_type))
            {
                auto projected_placement = co_await prv.type_placement_info(*projected_type);
                auto storage_placement = co_await prv.type_placement_info(storage_type);
                if (projected_placement.size > storage_placement.size || projected_placement.alignment > storage_placement.alignment)
                {
                    throw std::logic_error("Projected type does not fit in aligned storage");
                }
            }

            co_return ref_type;
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
            assert(!type_is_contextual(bind_type));
            codegen_binding binding;
            binding.attached_symbol = bind_type;
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

        auto create_reference(block_index& bidx, value_index index, type_symbol const& new_type)
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

        auto cast_reference(block_index bidx, value_index index, type_symbol const& ty)
        {
            auto temp = create_local_value(ty);
            vmir2::cast_reference ref;
            ref.source_ref_index = get_local_index(index);
            ref.target_ref_index = get_local_index(temp);
            this->emit(bidx, ref);
            return temp;
        }

        // Look up a type/class symbol in the current codegen context.
        // Uses co_lookup_symbol to respect local tempar type definitions.
        // Errors if the symbol resolves to a value binding or does not refer to a class.
        auto co_lookup_typeclass(block_index idx, type_symbol sym) -> typename CoroutineProvider::template co_type< type_symbol >
        {

            auto looked = co_await co_lookup_symbol(idx, sym);
            if (!looked.has_value())
            {
                throw std::logic_error("Type not found: " + to_string(sym));
            }

            auto val = looked.value();
            auto vtype = this->current_type(idx, val);

            if (!typeis< attached_type_reference >(vtype))
            {
                throw std::logic_error("Lookup did not yield a type symbol: " + to_string(vtype));
            }

            auto const& att = as< attached_type_reference >(vtype);
            // If carrying_type is not void, it's a member bound to a value.
            if (!typeis< void_type >(att.carrying_type))
            {
                throw std::logic_error("Type symbol bound to a value: " + to_string(vtype));
            }

            auto kind = co_await prv.symbol_type(att.attached_symbol);
            if (kind != symbol_kind::class_)
            {
                throw std::logic_error("Symbol is not a class: " + to_string(att.attached_symbol));
            }

            co_return att.attached_symbol;
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
                // std::cout << "lookup " << name << std::endl;
                auto lookup = this->local_value_direct_lookup(idx, name);
                if (lookup)
                {
                    auto lookup_type = this->current_type(idx, lookup.value());
                    // std::cout << "lookup " << name << " -> " << lookup.value() << " type=" << to_string(lookup_type) << std::endl;

                    if (!is_ref(lookup_type))
                    {
                        lookup = create_reference(idx, *lookup, make_mref(lookup_type));
                    }
                    else
                    {
                        lookup = copy_refernece_internal(idx, *lookup);

                        lookup_type = this->current_type(idx, *lookup);

                        auto const& lookup_type_ref = as< ptrref_type >(lookup_type);

                        if (lookup_type_ref.qual == qualifier::write || lookup_type_ref.qual == qualifier::constant)
                        {
                            lookup = cast_reference(idx, *lookup, make_mref(remove_ref(lookup_type)));
                        }
                    }

                    assert(!type_is_contextual(lookup_type));

                    co_return lookup;
                }
                else
                {
                    if (name == "t1")
                    {
                        int debugbrk = 0;
                        for (auto& [k, v] : this->state.scoped_definitions)
                        {
                            if (v.template type_is< type_symbol >())
                            {
                                auto def_type = v.template get_as< type_symbol >();
                                assert(!type_is_contextual(def_type));
                                std::cout << " scoped def: " << k << " = " << quxlang::to_string(def_type) << std::endl;
                            }
                            else
                            {
                                std::cout << " scoped def: " << k << " = CONSTEXPR" << std::endl;
                            }
                        }
                    }
                    if (this->state.scoped_definitions.contains(name))
                    {
                        auto const& def = this->state.scoped_definitions.at(name);
                        if (def.template type_is< type_symbol >())
                        {
                            auto def_type = def.template get_as< type_symbol >();
                            assert(!type_is_contextual(def_type));
                            auto binding = this->create_binding(value_index(0), def_type);
                            co_return binding;
                        }
                        else
                        {
                            // CONSTEXPR declaration
                            throw rpnx::unimplemented();
                        }
                    }
                }
            }
            auto canonical_symbol_opt = co_await prv.lookup(contextual_type_reference{.context = ctx, .type = sym});

            if (!canonical_symbol_opt)
            {
                co_return std::nullopt;
            }

            assert(!type_is_contextual(canonical_symbol_opt.value()));

            auto canonical_symbol = canonical_symbol_opt.value();
            std::cout << "co_lookup_symbol(" << symbol_str << ") -> " << quxlang::to_string(canonical_symbol) << std::endl;

            auto kind = co_await prv.symbol_type(canonical_symbol);

            value_index index(0);

            auto binding = this->create_binding(value_index(0), canonical_symbol);

            if (kind == quxlang::symbol_kind::global_variable)
            {
                auto global_get_reference = submember{.of = canonical_symbol, .name = "GET_REFERENCE"};
                index = co_await this->co_gen_call_functum(idx, global_get_reference, {});
            }
            else
            {
                index = binding;
            }

            co_return index;
        }

        auto co_gen_call_ctor(block_index& bidx, type_symbol new_type, codegen_invocation_args args) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto ctor = submember{.of = new_type, .name = "CONSTRUCTOR"};
            auto new_object = create_local_value(new_type);
            args.named["THIS"] = new_object;
            auto retval = co_await co_gen_call_functum(bidx, ctor, args);

            assert(retval == 0);

            auto dtor = co_await prv.class_default_dtor(new_type);
            if (dtor)
            {
                this->add_nontrivial_default_dtor(new_type, *dtor);
            }
            co_return new_object;
        }

        auto co_generate(block_index& bidx, expression_char_literal chr) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto number_string = std::to_string(static_cast< int >(chr.value));
            auto val = this->create_numeric_literal(number_string);
            assert(val != 0);
            auto val_type = this->current_type(bidx, val);
            QUXLANG_DEBUG({ std::cout << "Generated numeric literal from char " << val << " of type " << to_string(val_type) << std::endl; });
            co_return val;
        }

        auto co_generate(block_index& bidx, expression_call call) -> typename CoroutineProvider::template co_type< value_index >
        {
            std::cout << "gen_call_expr A()" << std::endl;
            auto callee = co_await co_generate_expr(bidx, call.callee);

            type_symbol callee_type = this->current_type(bidx, callee);
            std::cout << "gen_call_expr B() -> callee_type=" << quxlang::to_string(callee_type) << std::endl;

            std::string callee_type_string = to_string(callee_type);

            if (!typeis< attached_type_reference >(callee_type))
            {
                auto value_type = remove_ref(callee_type);
                auto operator_call = submember{.of = value_type, .name = "OPERATOR()"};
                callee = this->create_binding(callee, operator_call);
                callee_type = this->current_type(bidx, callee);
            }

            type_symbol attached_symbol = as< attached_type_reference >(callee_type).attached_symbol;

            type_symbol carrying_type = as< attached_type_reference >(callee_type).carrying_type;
            symbol_kind attached_symbol_kind = co_await prv.symbol_type(attached_symbol);

            codegen_invocation_args args;
            std::string callee_type_string2 = to_string(as< attached_type_reference >(callee_type));

            // std::cout << "requesting generate call to bindval=" << to_string(carrying_type) << " bindsym=" << to_string(attached_symbol) << std::endl;

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

            if (!typeis< void_type >(as< attached_type_reference >(callee_type).carrying_type))
            {
                auto const& callee_binding = this->state.genvalues.at(callee).template get_as< codegen_binding >();
                args.named["THIS"] = callee_binding.bound_value;
            }

            if (attached_symbol_kind == symbol_kind::class_)
            {
                if (!typeis< void_type >(as< attached_type_reference >(callee_type).carrying_type))
                {
                    throw std::logic_error("this is bug...");
                }
                auto object_type = as< attached_type_reference >(callee_type).attached_symbol;

                auto val = co_await co_gen_call_ctor(bidx, object_type, args);

                co_return val;
            }

            co_return co_await co_gen_call_functum(bidx, as< attached_type_reference >(callee_type).attached_symbol, args);
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
            std::vector< std::byte > value_bytes;
            for (char c : str)
            {
                value_bytes.push_back(static_cast< std::byte >(c));
            }
            lit.value = value_bytes;
            this->state.genvalues.push_back(lit);
            this->state.codegen_numeric_literals[str] = value_index(this->state.genvalues.size() - 1);
            return value_index(this->state.genvalues.size() - 1);
        }

        auto create_string_literal(std::string str)
        {
            if (auto it = this->state.codegen_string_literals.find(str); it != this->state.codegen_string_literals.end())
            {
                return it->second;
            }
            codegen_literal lit;
            lit.type = string_literal_reference{};
            std::vector< std::byte > value_bytes;
            for (char c : str)
            {
                value_bytes.push_back(static_cast< std::byte >(c));
            }
            lit.value = value_bytes;
            this->state.genvalues.push_back(lit);
            this->state.codegen_string_literals[str] = value_index(this->state.genvalues.size() - 1);
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

        auto co_gen_call_functanoid(block_index& bidx, instanciation_reference what, codegen_invocation_args expression_args, parameter_init_kind init_method) -> typename CoroutineProvider::template co_type< value_index >
        {
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

                if (arg_expr_type == arg_target_type)
                {
                    co_return arg_expr_index;
                }

                co_return co_await co_gen_argument_adaptation(bidx, arg_expr_index, arg_target_type, init_method);
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

            assert(!type_is_contextual(what));
            auto return_type = co_await prv.functanoid_return_type(what);

            // Index 0 is defined to be the special "void" value.
            value_index retval(0);

            if (!typeis< void_type >(return_type))
            {
                auto return_slot = create_local_value(return_type);
                // std::cout << "Created return slot " << return_slot << std::endl;

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

            co_return co_await co_gen_argument_adaptation(bidx, vidx, target_value_type, parameter_init_kind::call);
        }

        auto co_gen_implicit_conversion(block_index& bidx, value_index vidx, type_symbol target_type, std::optional< value_index > constructed_index = std::nullopt) -> typename CoroutineProvider::template co_type< value_index >
        {
            type_symbol value_type = this->current_type(bidx, vidx);
            // std::cout << "gen_implicit_conversion(" << vidx << "(" << to_string(value_type) << "), " << to_string(target_type) << ")" << std::endl;

            if (value_type == target_type)
            {
                assert(vidx != value_index(0));
                co_return vidx;
            }

            co_return co_await co_gen_argument_adaptation(bidx, vidx, target_type, parameter_init_kind::call);
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
            return of_type.type_is< int_type >() || of_type.type_is< bool_type >() || of_type.type_is< ptrref_type >() || of_type.type_is< array_type >() || of_type.type_is< byte_type >() || of_type.type_is< readonly_constant >();
        }

        // This implements builtin operators for primitives,
        // It assumes that we have already checked that the function is builtin and are just
        // checking which implementation to use.
        template < typename Inst >
        bool implement_binary_instruction(std::optional< vmir2::vm_instruction >& out, std::string const& operator_str, bool enable_rhs, submember const& member, invotype const& call, codegen_invocation_args const& args, bool flip = false)
        {
            bool is_normal = (member.name == "OPERATOR" + operator_str);
            bool is_rhs = (member.name == "OPERATOR" + operator_str + "RHS");
            if (is_normal || (is_rhs && enable_rhs))
            {

                if (call.named.contains("THIS") && call.named.contains("OTHER") && args.size() == 3)
                {
                    auto this_slot_id = get_local_index(args.named.at("THIS"));
                    auto other_slot_id = get_local_index(args.named.at("OTHER"));

                    Inst instr{};

                    instr.a = this_slot_id;
                    instr.b = other_slot_id;
                    // For RHS operator implementations, the operands are logically flipped (OTHER op THIS).
                    // Apply swap when either explicit flip is requested (for mapping >, >=) or when using RHS.
                    bool final_flip = flip ^ is_rhs;
                    if (final_flip)
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
                if (type.type_is< ptrref_type >())
                {
                    auto const& ptr = type.as< ptrref_type >();
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
                if (cls->template type_is< ptrref_type >() && cls->as< ptrref_type >().ptr_class != pointer_class::ref)
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

            if (member->name == "OPERATOR<->")
            {
                // defined for built in types without RHS

                if (call.named.contains("THIS") && call.named.contains("OTHER") && args.size() == 2)
                {
                    vmir2::swap swp;
                    swp.a = get_local_index(args.named.at("THIS"));
                    swp.b = get_local_index(args.named.at("OTHER"));
                    return swp;
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
                    if (cls->template type_is< readonly_constant >())
                    {
                        auto const ro = cls->as< readonly_constant >();
                        // Numeric literal to readonly constant
                        if (other.type_is< numeric_literal_reference >() && ro.kind == constant_kind::numeric)
                        {

                            auto other_slot_id = args.named.at("OTHER");
                            auto const& other_slot = this->state.genvalues.at(other_slot_id);

                            auto const& other_literal = other_slot.template get_as< codegen_literal >();
                            auto const& other_slot_value = other_literal.value;

                            vmir2::load_const_value lcv_result;
                            lcv_result.value = other_slot_value;
                            lcv_result.target = get_local_index(args.named.at("THIS"));
                            return lcv_result;
                        }

                        // String literal to readonly constant

                        else if (other.type_is< string_literal_reference >() && ro.kind == constant_kind::string)
                        {
                            auto other_slot_id = args.named.at("OTHER");

                            auto const& other_slot = this->state.genvalues.at(other_slot_id);

                            auto const& other_literal = other_slot.template get_as< codegen_literal >();
                            auto const& other_slot_value = other_literal.value;

                            vmir2::load_const_value lcv_result;
                            lcv_result.value = other_slot_value;
                            lcv_result.target = get_local_index(args.named.at("THIS"));
                            return lcv_result;
                        }
                    }
                    else if ((cls->template type_is< int_type >() || cls->template type_is< byte_type >()) && other.type_is< numeric_literal_reference >())
                    {
                        auto other_slot_id = args.named.at("OTHER");

                        auto const& other_slot = this->state.genvalues.at(other_slot_id);

                        assert(other_slot.template type_is< codegen_literal >());

                        auto const& other_literal = other_slot.template get_as< codegen_literal >();
                        auto const& other_slot_value = other_literal.value;

                        vmir2::load_const_int result;
                        for (auto byte : other_slot_value)
                        {
                            result.value.push_back(static_cast< char >(byte));
                        }
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
                    else if (cls->type_is< ptrref_type >() &&
                             other.type_is< ptrref_type >() &&
                             cls->as< ptrref_type >().ptr_class == pointer_class::ref &&
                             other.as< ptrref_type >().ptr_class == pointer_class::ref)
                    {
                        auto other_slot_id = args.named.at("OTHER");
                        auto this_slot_id = args.named.at("THIS");

                        auto const& other_ptrref = other.as< ptrref_type >();
                        auto const& cls_ptrref = cls->as< ptrref_type >();

                        assert(other_ptrref.ptr_class == cls_ptrref.ptr_class);
                        vmir2::cast_reference crf;
                        crf.source_ref_index = get_local_index(other_slot_id);
                        crf.target_ref_index = get_local_index(this_slot_id);
                        return crf;
                    }
                    else if (cls->type_is< ptrref_type >() && cls->as< ptrref_type >().ptr_class == pointer_class::ref)
                    {
                        auto materialized_type = remove_ref(*cls);
                        if (typeis< nvalue_slot >(materialized_type))
                        {
                            materialized_type = as< nvalue_slot >(materialized_type).target;
                        }
                        bool matches_materialized_value = (other == materialized_type);

                        if (matches_materialized_value)
                        {
                            auto other_slot_id = args.named.at("OTHER");
                            auto this_slot_id = args.named.at("THIS");

                            vmir2::make_reference mrf{};
                            mrf.value_index = get_local_index(other_slot_id);
                            mrf.reference_index = get_local_index(this_slot_id);

                            return mrf;
                        }
                    }
                }
                else if (args.size() == 1 && args.named.contains("THIS") && !cls->type_is< array_type >() && (!cls->type_is< ptrref_type >() || cls->as< ptrref_type >().ptr_class != pointer_class::ref))
                {
                    vmir2::load_const_zero result{};
                    result.target = get_local_index(args.named.at("THIS"));
                    return result;
                }
            }

            if (member->name == "OPERATOR:=")
            {
                if (cls->template type_is< int_type >() || cls->template type_is< bool_type >() || cls->template type_is< ptrref_type >())
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
                if (cls->template type_is< ptrref_type >())
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
                // Bitwise binary operators for integers
                if (implement_binary_instruction< vmir2::bitwise_and >(instr, "#&&", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::bitwise_or >(instr, "#||", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::bitwise_xor >(instr, "#^^", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::bitwise_nand >(instr, "#&!", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::bitwise_nor >(instr, "#|!", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::bitwise_nxor >(instr, "#^!", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::bitwise_implies >(instr, "#^->", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::bitwise_implied >(instr, "#^<-", true, *member, call, args))
                {
                    return instr;
                }

                // Bitwise shifts and rotates for integers (amount is uintptr)
                if (member->name == "OPERATOR#++" && call.named.contains("THIS") && call.named.contains("OTHER") && call.size() == 2)
                {
                    vmir2::bitwise_shift_up bi{};
                    bi.value = get_local_index(args.named.at("THIS"));
                    bi.amount = get_local_index(args.named.at("OTHER"));
                    bi.result = get_local_index(args.named.at("RETURN"));
                    return bi;
                }
                if (member->name == "OPERATOR#--" && call.named.contains("THIS") && call.named.contains("OTHER") && call.size() == 2)
                {
                    vmir2::bitwise_shift_down bi{};
                    bi.value = get_local_index(args.named.at("THIS"));
                    bi.amount = get_local_index(args.named.at("OTHER"));
                    bi.result = get_local_index(args.named.at("RETURN"));
                    return bi;
                }
                if (member->name == "OPERATOR#+%" && call.named.contains("THIS") && call.named.contains("OTHER") && call.size() == 2)
                {
                    vmir2::bitwise_rotate_up bi{};
                    bi.value = get_local_index(args.named.at("THIS"));
                    bi.amount = get_local_index(args.named.at("OTHER"));
                    bi.result = get_local_index(args.named.at("RETURN"));
                    return bi;
                }
                if (member->name == "OPERATOR#-%" && call.named.contains("THIS") && call.named.contains("OTHER") && call.size() == 2)
                {
                    vmir2::bitwise_rotate_down bi{};
                    bi.value = get_local_index(args.named.at("THIS"));
                    bi.amount = get_local_index(args.named.at("OTHER"));
                    bi.result = get_local_index(args.named.at("RETURN"));
                    return bi;
                }

                // Unary bitwise inverse for integers (suffix, non-RHS)
                if (member->name == "OPERATOR#!!" && call.named.contains("THIS") && call.size() == 1)
                {
                    vmir2::bitwise_inverse inv{};
                    inv.value = get_local_index(args.named.at("THIS"));
                    inv.result = get_local_index(args.named.at("RETURN"));
                    return inv;
                }
            }
            else if (cls->template type_is< byte_type >())
            {
                std::optional< vmir2::vm_instruction > instr;
                if (implement_binary_instruction< vmir2::cmp_eq >(instr, "==", true, *member, call, args))
                {
                    return instr;
                }
                else if (implement_binary_instruction< vmir2::cmp_ne >(instr, "!=", true, *member, call, args))
                {
                    return instr;
                }
                // Bitwise binary operators for bytes
                if (implement_binary_instruction< vmir2::bitwise_and >(instr, "#&&", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::bitwise_or >(instr, "#||", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::bitwise_xor >(instr, "#^^", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::bitwise_nand >(instr, "#&!", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::bitwise_nor >(instr, "#|!", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::bitwise_nxor >(instr, "#^!", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::bitwise_implies >(instr, "#^->", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::bitwise_implied >(instr, "#^<-", true, *member, call, args))
                {
                    return instr;
                }
                // Shifts and rotates for bytes
                if (member->name == "OPERATOR#++" && call.named.contains("THIS") && call.named.contains("OTHER") && call.size() == 2)
                {
                    vmir2::bitwise_shift_up bi{};
                    bi.value = get_local_index(args.named.at("THIS"));
                    bi.amount = get_local_index(args.named.at("OTHER"));
                    bi.result = get_local_index(args.named.at("RETURN"));
                    return bi;
                }
                if (member->name == "OPERATOR#--" && call.named.contains("THIS") && call.named.contains("OTHER") && call.size() == 2)
                {
                    vmir2::bitwise_shift_down bi{};
                    bi.value = get_local_index(args.named.at("THIS"));
                    bi.amount = get_local_index(args.named.at("OTHER"));
                    bi.result = get_local_index(args.named.at("RETURN"));
                    return bi;
                }
                if (member->name == "OPERATOR#+%" && call.named.contains("THIS") && call.named.contains("OTHER") && call.size() == 2)
                {
                    vmir2::bitwise_rotate_up bi{};
                    bi.value = get_local_index(args.named.at("THIS"));
                    bi.amount = get_local_index(args.named.at("OTHER"));
                    bi.result = get_local_index(args.named.at("RETURN"));
                    return bi;
                }
                if (member->name == "OPERATOR#-%" && call.named.contains("THIS") && call.named.contains("OTHER") && call.size() == 2)
                {
                    vmir2::bitwise_rotate_down bi{};
                    bi.value = get_local_index(args.named.at("THIS"));
                    bi.amount = get_local_index(args.named.at("OTHER"));
                    bi.result = get_local_index(args.named.at("RETURN"));
                    return bi;
                }
                // Unary bitwise inverse for bytes
                if (member->name == "OPERATOR#!!" && call.named.contains("THIS") && call.size() == 1)
                {
                    vmir2::bitwise_inverse inv{};
                    inv.value = get_local_index(args.named.at("THIS"));
                    inv.result = get_local_index(args.named.at("RETURN"));
                    return inv;
                }
            }
            else if (cls->template type_is< bool_type >())
            {
                std::optional< vmir2::vm_instruction > instr;
                if (implement_binary_instruction< vmir2::cmp_eq >(instr, "==", true, *member, call, args))
                {
                    return instr;
                }
                else if (implement_binary_instruction< vmir2::cmp_ne >(instr, "!=", true, *member, call, args))
                {
                    return instr;
                }
                else if (member->name == "OPERATOR!!" && call.named.contains("THIS") && call.size() == 1)
                {
                    vmir2::to_bool_not tbn{};
                    tbn.from = get_local_index(args.named.at("THIS"));
                    tbn.to = get_local_index(args.named.at("RETURN"));
                    return tbn;
                }
            }
            if (cls->template type_is< ptrref_type >() && (member->name == "OPERATOR+" || member->name == "OPERATOR-"))
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

                if (call.named.contains("THIS") && call.named.contains("OTHER") && call.named.at("OTHER").type_is< ptrref_type >() && call.size() == 2)
                {
                    vmir2::pointer_diff pdf;
                    pdf.from = get_local_index(args.named.at("THIS"));
                    pdf.to = get_local_index(args.named.at("OTHER"));
                    pdf.result = get_local_index(args.named.at("RETURN"));
                    return pdf;
                }
            }

            if (cls->template type_is< ptrref_type >())
            {
                auto const& ptrref = cls->as< ptrref_type >();
                auto operator_name = member->name;

                if (operator_name == "OPERATOR==")
                {
                    assert(args.named.contains("THIS"));
                    assert(args.named.contains("OTHER"));
                    assert(args.named.contains("RETURN"));
                    assert(args.size() == 3);

                    vmir2::pcmp_eq res;
                    res.a = get_local_index(args.named.at("THIS"));
                    res.b = get_local_index(args.named.at("OTHER"));
                    res.result = get_local_index(args.named.at("RETURN"));
                    return res;
                }
                if (operator_name == "OPERATOR!=")
                {
                    assert(args.named.contains("THIS"));
                    assert(args.named.contains("OTHER"));
                    assert(args.named.contains("RETURN"));
                    assert(args.size() == 3);

                    vmir2::pcmp_ne res;
                    res.a = get_local_index(args.named.at("THIS"));
                    res.b = get_local_index(args.named.at("OTHER"));
                    res.result = get_local_index(args.named.at("RETURN"));
                    return res;
                }
                if (operator_name == "OPERATOR<")
                {
                    assert(args.named.contains("THIS"));
                    assert(args.named.contains("OTHER"));
                    assert(args.named.contains("RETURN"));
                    assert(args.size() == 3);

                    vmir2::pcmp_lt res;
                    res.a = get_local_index(args.named.at("THIS"));
                    res.b = get_local_index(args.named.at("OTHER"));
                    res.result = get_local_index(args.named.at("RETURN"));
                    return res;
                }
                if (operator_name == "OPERATOR>")
                {
                    assert(args.named.contains("THIS"));
                    assert(args.named.contains("OTHER"));
                    assert(args.named.contains("RETURN"));
                    assert(args.size() == 3);

                    vmir2::pcmp_lt res;
                    res.b = get_local_index(args.named.at("THIS"));
                    res.a = get_local_index(args.named.at("OTHER"));
                    res.result = get_local_index(args.named.at("RETURN"));
                    return res;
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
            auto type_opt = co_await this->co_lookup_symbol(bidx, szof.of_type);
            if (!type_opt.has_value())
            {
                throw std::logic_error("Expected type " + quxlang::to_string(szof.of_type) + " to be defined.");
            }

            auto type_val = type_opt.value();

            auto const& genvalue = this->state.genvalues.at(type_val);

            if (genvalue.template type_is< codegen_literal >())
            {
                throw std::logic_error("Expected SIZEOF(...) to refer to a class type, got a literal genvalue instead (hint: cast to a concrete type like I32, NUMERIC_CONSTANT, STRING_CONSTANT, or similar).");
            }

            if (genvalue.template type_is< codegen_local >())
            {
                throw std::logic_error("Expected SIZEOF(...) to refer to a class type, got an object or reference instead.");
            }

            if (!genvalue.template type_is< codegen_binding >())
            {
                throw std::logic_error("Expected SIZEOF(...) to refer to a class type, got something else instead.");
            }
            auto const& binding = genvalue.template get_as< codegen_binding >();
            if (binding.bound_value != value_index(0))
            {
                throw std::logic_error("Expected SIZEOF(...) to refer to a class type, got an attached symbol (member function?) instead. (hint: cast member function attachments to a concrete type first)");
            }

            auto const& attached_type = binding.attached_symbol;
            assert(!type_is_contextual(attached_type));

            symbol_kind kind = co_await prv.symbol_type(attached_type);
            if (kind != symbol_kind::class_)
            {
                throw std::logic_error("Expected SIZEOF(...) to refer to a class type, got a non-class type instead.");
            }

            type_placement_info placement_info = co_await prv.type_placement_info(attached_type);

            auto lit = this->create_numeric_literal(std::to_string(placement_info.size));

            co_return lit;
        }

        auto co_generate(block_index& bidx, expression_bits szof) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto type_opt = co_await this->co_lookup_symbol(bidx, szof.of_type);
            if (!type_opt.has_value())
            {
                throw std::logic_error("Expected type " + quxlang::to_string(szof.of_type) + " to be defined.");
            }

            auto type_val = type_opt.value();

            auto const& genvalue = this->state.genvalues.at(type_val);

            if (genvalue.template type_is< codegen_literal >())
            {
                throw std::logic_error("Expected BITS(...) to refer to a integer type, got a literal genvalue instead (hint: cast to a concrete type like I32, NUMERIC_CONSTANT, STRING_CONSTANT, or similar).");
            }

            if (genvalue.template type_is< codegen_local >())
            {
                throw std::logic_error("Expected BITS(...) to refer to a integer type, got an object or reference instead.");
            }

            if (!genvalue.template type_is< codegen_binding >())
            {
                throw std::logic_error("Expected BITS(...) to refer to a integer type, got something else instead.");
            }
            auto const& binding = genvalue.template get_as< codegen_binding >();
            if (binding.bound_value != value_index(0))
            {
                throw std::logic_error("Expected BITS(...) to refer to a integer type, got an attached symbol (member function?) instead. (hint: cast member function attachments to a concrete type first)");
            }

            type_symbol const& attached_type = binding.attached_symbol;
            assert(!type_is_contextual(attached_type));

            symbol_kind kind = co_await prv.symbol_type(attached_type);
            if (kind != symbol_kind::class_)
            {
                throw std::logic_error("Expected BITS(...) to refer to an integer type, got a non-class type instead.");
            }

            if (!attached_type.template type_is< int_type >())
            {
                throw std::logic_error("Expected BITS(...) to refer to an integer type, got a non-integer class type instead.");
            }

            int_type const& inttype = attached_type.template as< int_type >();
            auto lit = this->create_numeric_literal(std::to_string(inttype.bits));

            co_return lit;
        }

        auto co_generate(block_index& bidx, expression_is_signed szof) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto type_opt = co_await this->co_lookup_symbol(bidx, szof.of_type);
            if (!type_opt.has_value())
            {
                throw std::logic_error("Expected type " + quxlang::to_string(szof.of_type) + " to be defined.");
            }

            auto type_val = type_opt.value();

            auto const& genvalue = this->state.genvalues.at(type_val);

            if (genvalue.template type_is< codegen_literal >())
            {
                throw std::logic_error("Expected BITS(...) to refer to a integer type, got a literal genvalue instead (hint: cast to a concrete type like I32, NUMERIC_CONSTANT, STRING_CONSTANT, or similar).");
            }

            if (genvalue.template type_is< codegen_local >())
            {
                throw std::logic_error("Expected BITS(...) to refer to a integer type, got an object or reference instead.");
            }

            if (!genvalue.template type_is< codegen_binding >())
            {
                throw std::logic_error("Expected BITS(...) to refer to a integer type, got something else instead.");
            }
            auto const& binding = genvalue.template get_as< codegen_binding >();
            if (binding.bound_value != value_index(0))
            {
                throw std::logic_error("Expected BITS(...) to refer to a integer type, got an attached symbol (member function?) instead. (hint: cast member function attachments to a concrete type first)");
            }

            type_symbol const& attached_type = binding.attached_symbol;
            assert(!type_is_contextual(attached_type));

            symbol_kind kind = co_await prv.symbol_type(attached_type);
            if (kind != symbol_kind::class_)
            {
                throw std::logic_error("Expected BITS(...) to refer to an integer type, got a non-class type instead.");
            }

            if (attached_type.template type_is< byte_type >())
            {
                co_return this->create_bool_value(bidx, false);
            }

            if (!attached_type.template type_is< int_type >())
            {
                throw std::logic_error("Expected BITS(...) to refer to an integer type, got a non-integer class type instead.");
            }

            int_type const& inttype = attached_type.template as< int_type >();
            co_return this->create_bool_value(bidx, inttype.has_sign);
        }

        auto co_generate(block_index& bidx, expression_is_integral szof) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto type_opt = co_await this->co_lookup_symbol(bidx, szof.of_type);
            if (!type_opt.has_value())
            {
                throw std::logic_error("Expected type " + quxlang::to_string(szof.of_type) + " to be defined.");
            }

            auto type_val = type_opt.value();

            auto const& genvalue = this->state.genvalues.at(type_val);

            if (genvalue.template type_is< codegen_literal >())
            {
                co_return this->create_bool_value(bidx, false);
            }

            if (genvalue.template type_is< codegen_local >())
            {
                throw std::logic_error("Expected IS_INTEGRAL(...) to refer to a type, got an object or reference instead.");
            }

            if (!genvalue.template type_is< codegen_binding >())
            {
                throw std::logic_error("Expected IS_INTEGRAL(...) to refer to a type, got something else instead.");
            }
            auto const& binding = genvalue.template get_as< codegen_binding >();
            if (binding.bound_value != value_index(0))
            {
                throw std::logic_error("Expected IS_INTEGRAL(...) to refer to a type, got an attached symbol (member function?) instead. (hint: cast member function attachments to a concrete type first)");
            }

            type_symbol const& attached_type = binding.attached_symbol;
            assert(!type_is_contextual(attached_type));

            symbol_kind kind = co_await prv.symbol_type(attached_type);
            if (kind != symbol_kind::class_)
            {
                co_return this->create_bool_value(bidx, false);
            }

            if (!typeis_oneof< int_type, byte_type >(attached_type))
            {
                co_return this->create_bool_value(bidx, false);
            }

            co_return this->create_bool_value(bidx, true);
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

            auto pointer_storage = create_local_value(ptrref_type{.target = non_ref_type, .ptr_class = pointer_class::instance, .qual = qualifier::mut});

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

            if (kw.keyword == "THIS" || kw.keyword == "OTHER")
            {
                auto result = co_await this->co_lookup_symbol(bidx, freebound_identifier{.name = kw.keyword});
                if (!result.has_value())
                {
                    throw std::runtime_error("Expected symbol " + kw.keyword + " to be defined.");
                }
                co_return result.value();
            }

            throw rpnx::unimplemented();
        }

        auto co_generate(block_index& bidx, expression_static_choose const& sc) -> typename CoroutineProvider::template co_type< value_index >
        {
            bool res = co_await co_constexpr_bool(bidx, sc.condition);
            if (res)
            {
                co_return co_await co_generate_expr(bidx, sc.true_expr);
            }
            else
            {
                co_return co_await co_generate_expr(bidx, sc.false_expr);
            }
        }

        // Runtime CHOOSE(cond, true_expr, false_expr): evaluate condition at runtime and pick a value
        auto co_generate(block_index& bidx, expression_choose const& ch) -> typename CoroutineProvider::template co_type< value_index >
        {
            // Create control-flow blocks
            auto after_block = this->generate_subblock(bidx, "choose_after");
            auto condition_block = this->generate_subblock(bidx, "choose_condition");
            auto true_block = this->generate_subblock(bidx, "choose_true");
            auto false_block = this->generate_subblock(bidx, "choose_false");

            auto true_block_init = true_block;
            auto false_block_init = false_block;

            // Jump into condition evaluation
            this->generate_jump(bidx, condition_block);

            auto cond = co_await co_generate_bool_expr(condition_block, ch.condition);

            this->generate_branch(cond, condition_block, true_block, false_block);
            // Generate condition as bool and branch

            auto val1 = co_await this->co_generate_expr(true_block, ch.true_expr);
            auto val2 = co_await this->co_generate_expr(false_block, ch.false_expr);

            co_await this->co_converge_values(after_block, true_block_init, val1, false_block_init, val2);

            throw rpnx::unimplemented();
        }

        // co_converge_values causes two distinct values on different blocks to converge into one value
        // in the output block.

        auto co_converge_values(block_index& output_block, block_index& bidx1, value_index val1, block_index& bidx2, value_index val2) -> typename CoroutineProvider::template co_type< value_index >
        {
            throw rpnx::unimplemented();
        }

        auto co_constexpr_bool(block_index&, expression const& expr) -> typename CoroutineProvider::template co_type< bool >
        {
            // TODO: Add carried context support
            auto ce_input = constexpr_input{.context = ctx, .expr = expr};
            // TODO: ce_input.scoped_definitions = ...;
            auto ce_result = co_await prv.constexpr_bool(ce_input);
            co_return ce_result;
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

        auto co_generate_logic_or(block_index& bidx, expression_binary input) -> typename CoroutineProvider::template co_type< value_index >
        {
            assert(bidx == block_index(0) || this->state.blocks.at(0).terminator.has_value());
            auto result_bool = this->create_local_value(bool_type{});
            auto lhs_block = this->generate_subblock(bidx, "logic_or_lhs");
            this->generate_jump(bidx, lhs_block);
            auto false_block = this->generate_subblock(bidx, "logic_or_false");
            auto true_block = this->generate_subblock(bidx, "logic_or_true");
            auto after_block = this->generate_subblock(bidx, "logic_or_after");
            auto lhs = co_await co_generate_bool_expr(lhs_block, input.lhs);
            auto rhs_block = this->generate_subblock(lhs_block, "logic_or_rhs");
            this->generate_branch(lhs, lhs_block, true_block, rhs_block);
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

        auto co_generate_logic_nor(block_index& bidx, expression_binary input) -> typename CoroutineProvider::template co_type< value_index >
        {
            assert(bidx == block_index(0) || this->state.blocks.at(0).terminator.has_value());
            auto result_bool = this->create_local_value(bool_type{});
            auto lhs_block = this->generate_subblock(bidx, "logic_nor_lhs");
            this->generate_jump(bidx, lhs_block);
            auto false_block = this->generate_subblock(bidx, "logic_nor_false");
            auto true_block = this->generate_subblock(bidx, "logic_nor_true");
            auto after_block = this->generate_subblock(bidx, "logic_nor_after");
            auto lhs = co_await co_generate_bool_expr(lhs_block, input.lhs);
            auto rhs_block = this->generate_subblock(lhs_block, "logic_nor_rhs");
            this->generate_branch(lhs, lhs_block, false_block, rhs_block);
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

        auto co_generate_logic_xor(block_index& bidx, expression_binary input) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto lhs = co_await co_generate_bool_expr(bidx, input.lhs);
            auto rhs = co_await co_generate_bool_expr(bidx, input.rhs);
            co_return co_await co_generate_binary(bidx, "!=", lhs, rhs);
        }

        auto co_generate_logic_nxor(block_index& bidx, expression_binary input) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto lhs = co_await co_generate_bool_expr(bidx, input.lhs);
            auto rhs = co_await co_generate_bool_expr(bidx, input.rhs);
            co_return co_await co_generate_binary(bidx, "==", lhs, rhs);
        }

        auto co_generate_logic_implies(block_index& bidx, expression_binary input) -> typename CoroutineProvider::template co_type< value_index >
        {
            assert(bidx == block_index(0) || this->state.blocks.at(0).terminator.has_value());
            auto result_bool = this->create_local_value(bool_type{});
            auto lhs_block = this->generate_subblock(bidx, "logic_implies_lhs");
            this->generate_jump(bidx, lhs_block);
            auto false_block = this->generate_subblock(bidx, "logic_implies_false");
            auto true_block = this->generate_subblock(bidx, "logic_implies_true");
            auto after_block = this->generate_subblock(bidx, "logic_implies_after");
            auto lhs = co_await co_generate_bool_expr(lhs_block, input.lhs);
            auto rhs_block = this->generate_subblock(lhs_block, "logic_implies_rhs");
            this->generate_branch(lhs, lhs_block, rhs_block, true_block);
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

        auto co_generate_logic_implied(block_index& bidx, expression_binary input) -> typename CoroutineProvider::template co_type< value_index >
        {
            assert(bidx == block_index(0) || this->state.blocks.at(0).terminator.has_value());
            auto result_bool = this->create_local_value(bool_type{});
            auto lhs_block = this->generate_subblock(bidx, "logic_implied_lhs");
            this->generate_jump(bidx, lhs_block);
            auto false_block = this->generate_subblock(bidx, "logic_implied_false");
            auto true_block = this->generate_subblock(bidx, "logic_implied_true");
            auto after_block = this->generate_subblock(bidx, "logic_implied_after");
            auto lhs = co_await co_generate_bool_expr(lhs_block, input.lhs);
            auto rhs_block = this->generate_subblock(lhs_block, "logic_implied_rhs");
            this->generate_branch(lhs, lhs_block, true_block, rhs_block);
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

        auto co_generate_binary(block_index& bidx, std::string operator_str, value_index lhs, value_index rhs) -> typename CoroutineProvider::template co_type< value_index >
        {
            type_symbol lhs_type = this->current_type(bidx, lhs);
            type_symbol rhs_type = this->current_type(bidx, rhs);

            type_symbol lhs_underlying_type = remove_ref(lhs_type);
            type_symbol rhs_underlying_type = remove_ref(rhs_type);

            type_symbol lhs_function = submember{lhs_underlying_type, "OPERATOR" + operator_str};
            type_symbol rhs_function = submember{rhs_underlying_type, "OPERATOR" + operator_str + "RHS"};
            invotype lhs_param_info{.named = {{"THIS", lhs_type}, {"OTHER", rhs_type}}};
            invotype rhs_param_info{.named = {{"THIS", rhs_type}, {"OTHER", lhs_type}}};

            auto lhs_exists_and_callable_with = co_await prv.instanciation(initialization_reference{.initializee = lhs_function, .parameters = lhs_param_info, .init_kind = parameter_init_kind::call});

            if (lhs_exists_and_callable_with)
            {
                auto lhs_args = codegen_invocation_args{.named = {{"THIS", lhs}, {"OTHER", rhs}}};
                co_return co_await co_gen_call_functum(bidx, lhs_function, lhs_args);
            }

            auto rhs_exists_and_callable_with = co_await prv.instanciation(initialization_reference{.initializee = rhs_function, .parameters = rhs_param_info, parameter_init_kind::call});

            if (rhs_exists_and_callable_with)
            {
                auto rhs_args = codegen_invocation_args{.named = {{"THIS", rhs}, {"OTHER", lhs}}};
                co_return co_await co_gen_call_functum(bidx, rhs_function, rhs_args);
            }

            throw std::logic_error("Found neither " + to_string(lhs_function) + " callable with (" + to_string(lhs_type) + ", " + to_string(rhs_type) + ") nor " + to_string(rhs_function) + " callable with (" + to_string(rhs_type) + ", " + to_string(lhs_type) + ")");
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

                if (input.operator_str == "||")
                {
                    co_return co_await co_generate_logic_or(bidx, input);
                }

                if (input.operator_str == "!|")
                {
                    co_return co_await co_generate_logic_nor(bidx, input);
                }

                if (input.operator_str == "^^")
                {
                    co_return co_await co_generate_logic_xor(bidx, input);
                }

                if (input.operator_str == "!^")
                {
                    co_return co_await co_generate_logic_nxor(bidx, input);
                }

                if (input.operator_str == "^>")
                {
                    co_return co_await co_generate_logic_implies(bidx, input);
                }

                if (input.operator_str == "^<")
                {
                    co_return co_await co_generate_logic_implied(bidx, input);
                }
            }
            auto lhs = co_await co_generate_expr(bidx, input.lhs);
            auto rhs = co_await co_generate_expr(bidx, input.rhs);

            co_return co_await co_generate_binary(bidx, input.operator_str, lhs, rhs);
        }

        auto co_generate(block_index& bidx, expression_numeric_literal input) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto val = this->create_numeric_literal(input.value);
            assert(val != 0);
            auto val_type = this->current_type(bidx, val);
            QUXLANG_DEBUG({ std::cout << "Generated numeric literal " << val << " of type " << to_string(val_type) << std::endl; });
            co_return val;
        }

        auto co_generate(block_index& bidx, expression_string_literal input) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto val = this->create_string_literal(input.value);
            assert(val != 0);
            auto val_type = this->current_type(bidx, val);
            QUXLANG_DEBUG({ std::cout << "Generated string literal " << val << " of type " << to_string(val_type) << std::endl; });
            co_return val;
        }

        auto co_begin_storage_delegate(block_index& bidx, value_index storage_ref, type_symbol target_type, bool destroy_delegate) -> typename CoroutineProvider::template co_type< value_index >
        {
            co_await co_expect_storage_reference(bidx, storage_ref, true, target_type);

            auto delegate_value = this->create_local_value(target_type);
            if (destroy_delegate)
            {
                this->emit(bidx, vmir2::storage_deinit_start{
                                     .on_storage = get_local_index(storage_ref),
                                     .target_value = get_local_index(delegate_value)});
            }
            else
            {
                this->emit(bidx, vmir2::storage_init_start{
                                     .on_storage = get_local_index(storage_ref),
                                     .target_value = get_local_index(delegate_value)});
            }

            co_return delegate_value;
        }

        auto co_generate_place_expression_impl(block_index& bidx, value_index storage_ref, type_symbol target_type, std::optional< expression > const& assign_init, std::vector< expression_arg > const& args_in) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto storage_ref_type = co_await co_expect_storage_reference(bidx, storage_ref, true, target_type);
            auto constructor = submember{.of = target_type, .name = "CONSTRUCTOR"};
            codegen_invocation_args ctor_args;
            auto storage_delegate = co_await co_begin_storage_delegate(bidx, storage_ref, target_type, false);
            ctor_args.named["THIS"] = storage_delegate;

            if (assign_init.has_value())
            {
                auto init_val = co_await co_generate_expr(bidx, *assign_init);
                ctor_args.named["OTHER"] = init_val;
            }
            else
            {
                for (auto const& arg : args_in)
                {
                    auto arg_val = co_await co_generate_expr(bidx, arg.value);
                    if (arg.name.has_value())
                    {
                        ctor_args.named[*arg.name] = arg_val;
                    }
                    else
                    {
                        ctor_args.positional.push_back(arg_val);
                    }
                }
            }

            co_await this->co_gen_call_functum(bidx, constructor, ctor_args);

            auto typed_ref = this->create_local_value(ptrref_type{.target = target_type, .ptr_class = pointer_class::ref, .qual = storage_ref_type.qual});
            this->emit(bidx, vmir2::storage_pun{
                                 .from_storage = get_local_index(storage_ref),
                                 .as_type = target_type,
                                 .to_reference = get_local_index(typed_ref)});

            auto result_pointer = create_local_value(ptrref_type{.target = target_type, .ptr_class = pointer_class::instance, .qual = storage_ref_type.qual});
            this->emit(bidx, vmir2::make_pointer_to{
                                 .of_index = get_local_index(typed_ref),
                                 .pointer_index = get_local_index(result_pointer)});
            co_return result_pointer;
        }

        auto co_generate_place_expression(block_index& bidx, expression const& at_expr, type_symbol const& parsed_type, std::optional< expression > const& assign_init, std::vector< expression_arg > const& args_in) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto target_type = co_await co_lookup_typeclass(bidx, parsed_type);
            auto storage_ref = co_await co_generate_expr(bidx, at_expr);
            co_return co_await co_generate_place_expression_impl(bidx, storage_ref, target_type, assign_init, args_in);
        }

        auto co_generate(block_index& bidx, expression_typecast input) -> typename CoroutineProvider::template co_type< value_index >
        {
            // Conversions call the destination type's constructor with a named argument.
            // Default name is "OTHER"; if a keyword is present (e.g., NARROWING/WRAP/CHECKED), use that instead.
            auto arg_val = co_await co_generate_expr(bidx, input.expr);

            // Resolve the target class/type using local scope-aware lookup that supports tempar types.
            type_symbol target_class = co_await co_lookup_typeclass(bidx, input.to_type);

            codegen_invocation_args args;
            auto name = input.keyword.has_value() ? *input.keyword : std::string("OTHER");
            args.named[name] = arg_val;

            co_return co_await co_gen_call_ctor(bidx, target_class, args);
        }

        auto co_generate(block_index& bidx, expression_pun input) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto storage_ref = co_await co_generate_expr(bidx, input.value);
            auto target_type = co_await co_lookup_typeclass(bidx, input.as_type);
            auto storage_ref_type = co_await co_expect_storage_reference(bidx, storage_ref, false, target_type);
            auto result_ref = create_local_value(ptrref_type{.target = target_type, .ptr_class = pointer_class::ref, .qual = storage_ref_type.qual});
            this->emit(bidx, vmir2::storage_pun{.from_storage = get_local_index(storage_ref), .as_type = target_type, .to_reference = get_local_index(result_ref)});
            co_return result_ref;
        }

        auto co_generate(block_index& bidx, expression_place input) -> typename CoroutineProvider::template co_type< value_index >
        {
            co_return co_await co_generate_place_expression(bidx, input.at, input.type, input.assign_init, input.args);
        }

        auto co_generate(block_index& bidx, expression_unary_postfix input) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto val = co_await co_generate_expr(bidx, input.lhs);
            co_return co_await co_generate_unary_postfix(bidx, input.operator_str, val);
        }

        auto co_generate_unary_postfix(block_index& bidx, std::string operator_str, value_index val) -> typename CoroutineProvider::template co_type< value_index >
        {
            auto oper = this->get_class_member(bidx, val, "OPERATOR" + operator_str);
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

            if (!st.else_block.has_value())
            {
                this->generate_branch(cond, condition_block, if_block, after_block);

                // Then
                co_await co_generate_function_block(if_block, st.then_block, "if_then");
                this->generate_jump(if_block, after_block);
            }
            else
            {
                block_index else_block = this->generate_subblock(current_block, "if_statement_else");
                this->generate_branch(cond, condition_block, if_block, else_block);

                // Then
                co_await co_generate_function_block(if_block, st.then_block, "if_then");
                this->generate_jump(if_block, after_block);

                // Else
                co_await co_generate_function_block(else_block, *st.else_block, "if_else");
                this->generate_jump(else_block, after_block);
            }

            current_block = after_block;

            co_return;
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_unimplemented_statement const& st) -> typename CoroutineProvider::template co_type< void >
        {
            this->emit(current_block, vmir2::unimplemented{.message = st.error_message});
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

        auto generate_survivor_local_chain(block_index from, block_index it, block_index end, local_index survivor) -> void
        {
            this->block(it).entry_state[survivor] = this->block(from).current_state[survivor];
            this->block(it).current_state[survivor] = this->block(from).current_state[survivor];

            if (it == end)
            {
                return;
            }

            rpnx::apply_visitor< void >(
                this->block(it).terminator.value(),
                [&](auto const& term)
                {
                    using T = std::remove_cvref_t< decltype(term) >;
                    if constexpr (std::is_same_v< T, vmir2::branch >)
                    {
                        generate_survivor_local_chain(it, term.target_true, end, survivor);
                        generate_survivor_local_chain(it, term.target_false, end, survivor);
                    }
                    else if constexpr (std::is_same_v< T, vmir2::jump >)
                    {
                        generate_survivor_local_chain(it, term.target, end, survivor);
                    }
                });
        }

        auto generate_survivor_local(block_index from, block_index to, local_index survivor) -> void
        {
            this->block(to).entry_state[survivor] = this->block(from).current_state[survivor];
            this->block(to).current_state[survivor] = this->block(from).current_state[survivor];
            assert(this->block(to).instructions.empty());
        }

        // Helper: allow marking a local as surviving into a block even after that block has instructions
        // This is useful when a local is created in one branch and needs to be visible in another branch post-generation.
        auto generate_survivor_local_post(block_index from, block_index to, local_index survivor) -> void
        {
            this->block(to).entry_state[survivor] = this->block(from).current_state[survivor];
            this->block(to).current_state[survivor] = this->block(from).current_state[survivor];
            // Intentionally no assert on to.instructions emptiness
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

            if (st.equals_initializer.has_value())
            {
                auto init_idx = co_await co_generate_expr(new_expr_block, *st.equals_initializer);
                args.named["OTHER"] = init_idx;
            }

            if (typeis< storage >(var_type) || typeis< aligned_storage >(var_type))
            {
                if (!st.initializers.empty() || st.equals_initializer.has_value())
                {
                    throw std::logic_error("STORAGE variables do not support direct initializers");
                }
                this->emit(new_expr_block, vmir2::storage_init{.storage = get_local_index(idx)});
            }
            else
            {
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
                    type_symbol result_ref_type = recast_reference(base_type.template get_as< ptrref_type >(), field.type);
                    auto result_idx = create_local_value(result_ref_type);
                    access.store_index = get_local_index(result_idx);
                    // std::cout << "Created field access " << access.store_index << " for " << field_name << " in " << to_string(base_type) << std::endl;

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
            assert(!type_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();
            block_index current_block = block_index(0);
            bool type_is_array;
            auto cls = func.temploid.templexoid.get_as< submember >().of;
            if (cls.template type_is< array_type >())
            {
                co_await co_generate_array_ctor_delegates(current_block, func, {});
            }
            else
            {
                co_await co_generate_struct_ctor_delegates(current_block, func, {});
            }
            co_await co_generate_builtin_return(current_block);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_swap(instanciation_reference const& func) -> typename CoroutineProvider::template co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();
            block_index current_block = block_index(0);
            co_await co_generate_swap_members(current_block, func);
            co_await co_generate_builtin_return(current_block);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_global_init(instanciation_reference const& func) -> typename CoroutineProvider::template co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();
            block_index current_block = block_index(0);

            auto global_symbol = func.temploid.templexoid.get_as< submember >().of;
            auto global_type = co_await prv.variable_type(global_symbol);
            auto decl = co_await prv.symboid(global_symbol);
            if (!typeis< ast2_variable_declaration >(decl))
            {
                throw compiler_bug("Global variable declaration not found");
            }

            auto const& variable_decl = as< ast2_variable_declaration >(decl);
            auto storage_ref = (co_await this->co_lookup_symbol(current_block, freebound_identifier{"STORAGE"})).value();
            auto ignored = co_await co_generate_place_expression_impl(current_block, storage_ref, global_type, variable_decl.init_expr, variable_decl.init_args);
            (void)ignored;

            co_await co_generate_builtin_return(current_block);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_global_get_reference(instanciation_reference const& func) -> typename CoroutineProvider::template co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();

            auto entry_block = block_index(0);
            auto global_symbol = func.temploid.templexoid.get_as< submember >().of;
            auto global_type = co_await prv.variable_type(global_symbol);

            storage global_storage_type;
            global_storage_type.storable_types.insert(global_type);

            auto lock_value = create_local_value(initguard_lock_type{});
            auto initialized_block = this->generate_subblock(entry_block, "global_already_initialized");
            auto acquire_block = this->generate_subblock(entry_block, "global_acquired");

            if (this->state.blocks.at(entry_block).terminator.has_value())
            {
                throw compiler_bug("Expected no terminator in global GET_REFERENCE entry block");
            }

            this->state.blocks.at(entry_block).terminator = vmir2::initguard_try_acquire{
                .symbol = global_symbol,
                .target_lock = get_local_index(lock_value),
                .target_acquired = acquire_block,
                .target_already_initialized = initialized_block,
            };

            vmir2::slot_state lock_state;
            lock_state.stage = vmir2::slot_stage::full;
            lock_state.storage_valid = true;
            this->block(acquire_block).entry_state[get_local_index(lock_value)] = lock_state;
            this->block(acquire_block).current_state[get_local_index(lock_value)] = lock_state;

            auto emit_return_from_storage = [&](block_index& current_block) -> typename CoroutineProvider::template co_type< void >
            {
                auto storage_ref = this->create_local_value(make_mref(global_storage_type));
                this->emit(current_block, vmir2::get_global_storage{
                    .symbol = global_symbol,
                    .target_ref = get_local_index(storage_ref),
                });

                auto result_ref = this->create_local_value(make_mref(global_type));
                this->emit(current_block, vmir2::storage_pun{
                    .from_storage = get_local_index(storage_ref),
                    .as_type = global_type,
                    .to_reference = get_local_index(result_ref),
                });

                co_await this->co_return_value(current_block, result_ref);
            };

            auto init_functum = submember{.of = global_symbol, .name = "INIT"};
            auto init_storage_ref = this->create_local_value(make_mref(global_storage_type));
            this->emit(acquire_block, vmir2::get_global_storage{
                .symbol = global_symbol,
                .target_ref = get_local_index(init_storage_ref),
            });
            co_await this->co_gen_call_functum(acquire_block, init_functum, codegen_invocation_args{.named = {{"STORAGE", init_storage_ref}}});
            this->emit(acquire_block, vmir2::initguard_release{.lock = get_local_index(lock_value)});
            co_await emit_return_from_storage(acquire_block);

            co_await emit_return_from_storage(initialized_block);

            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_access_member(instanciation_reference const& func, std::string const& member_name) -> typename CoroutineProvider::template co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();
            block_index current_block = block_index(0);

            auto thisval = (co_await this->co_lookup_symbol(current_block, freebound_identifier{"THIS"})).value();

            auto retval = co_await this->co_generate_dot_access(current_block, thisval, member_name);
            std::string retval_type_str = quxlang::to_string(this->current_type(current_block, retval));

            co_await this->co_return_value(current_block, retval);

            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_serialize(instanciation_reference const& func) -> typename CoroutineProvider::template co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));

            type_symbol class_type = func.temploid.templexoid.get_as< submember >().of;

            if (class_type.type_is< int_type >() || class_type.type_is< byte_type >())
            {
                co_return co_await this->co_generate_builtin_serialize_int(func);
            }

            throw rpnx::unimplemented();
        }

        auto co_generate_builtin_serialize_int(instanciation_reference const& func) -> typename CoroutineProvider::template co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            type_symbol class_type = func.temploid.templexoid.get_as< submember >().of;

            assert(class_type.type_is< int_type >() || class_type.type_is< byte_type >());
            co_await co_generate_arg_info(func);
            this->generate_entry_block();

            auto current_block = block_index(0);
            std::size_t bits = 8;
            bool signed_bits = false;
            if (class_type.type_is< int_type >())
            {
                int_type const& class_type_int = class_type.unwrap< int_type >();

                bits = class_type_int.bits;
                signed_bits = class_type_int.has_sign;
            }
            auto this_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"THIS"});

            auto copy_val = this->create_local_value(class_type);

            auto val_ctor = submember{.of = class_type, .name = "CONSTRUCTOR"};

            co_await co_gen_call_functum(current_block, val_ctor, codegen_invocation_args{.named = {{"OTHER", this_ref.value()}, {"THIS", copy_val}}}, parameter_init_kind::implicit_conversion);

            auto class_mreftype = make_mref(class_type);

            for (std::size_t i = 0; i < bits; i += 8)
            {
                // Load current state of copyval into a local, duplicating the original value
                auto copymutref = this->create_reference(current_block, copy_val, class_mreftype);
                auto copy_val_copy = this->create_local_value(class_type);
                {
                    vmir2::load_from_ref lfr;
                    lfr.from_reference = get_local_index(copymutref);
                    lfr.to_value = get_local_index(copy_val_copy);
                    this->emit(current_block, lfr);
                }

                // iter++
                auto outit_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"OUTPUT_ITERATOR"});
                if (!outit_ref.has_value())
                {
                    throw std::logic_error("Cannot find __out_iter in serialize");
                }
                auto incr = co_await co_generate_unary_postfix(current_block, "++", outit_ref.value());

                // (iter++)->
                auto outit_deref = co_await co_generate_unary_postfix(current_block, "->", incr);

                value_index byteval;

                if (class_type != byte_type{})
                {
                    byteval = create_local_value(byte_type{});
                    vmir2::iconv icv;
                    icv.convtype = vmir2::conversion_class::partial;
                    icv.from = get_local_index(copy_val_copy);
                    icv.to = get_local_index(byteval);
                    emit(current_block, icv);
                }
                else
                {
                    byteval = copy_val_copy;
                }

                // (iter++)-> := copy_val_copy;
                co_await co_generate_binary(current_block, ":=", outit_deref, byteval);

                copymutref = this->create_reference(current_block, copy_val, class_mreftype);
                copy_val_copy = this->create_local_value(class_type);
                {
                    vmir2::load_from_ref lfr;
                    lfr.from_reference = get_local_index(copymutref);
                    lfr.to_value = get_local_index(copy_val_copy);
                    this->emit(current_block, lfr);
                }

                // (val #-- 8)
                auto eight = create_numeric_literal("8");
                auto shifted_val = co_await co_generate_binary(current_block, "#--", copy_val_copy, eight);

                // copyval := copyval #-- 8
                copymutref = this->create_reference(current_block, copy_val, class_mreftype);
                co_await co_generate_binary(current_block, ":=", copymutref, shifted_val);
            }
            auto outit_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"OUTPUT_ITERATOR"});
            if (!outit_ref.has_value())
            {
                throw compiler_bug("Shouldn't be possible");
            }
            co_await co_return_value(current_block, *outit_ref);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_copy_ctor(instanciation_reference const& func) -> typename CoroutineProvider::template co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            auto class_type = func.temploid.templexoid.get_as< submember >().of;
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
            assert(!type_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();
            block_index current_block = block_index(0);
            co_await co_generate_move_ctor_delegates(current_block, func);
            co_await co_generate_builtin_return(current_block);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_assignment(instanciation_reference const& func) -> typename CoroutineProvider::template co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();
            block_index current_block = block_index(0);
            auto swap_expr = expression_binary{
                .operator_str = "<->",
                .lhs = expression_symbol_reference{.symbol = freebound_identifier{"THIS"}},
                .rhs = expression_symbol_reference{.symbol = freebound_identifier{"OTHER"}},
            };
            co_await co_generate(current_block, swap_expr);
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
                if (typeis< storage >(slot.type) || typeis< aligned_storage >(slot.type))
                {
                    continue;
                }
                auto dtor = co_await prv.class_default_dtor(slot.type);
                if (dtor)
                {
                    assert(!this->state.non_trivial_dtors.contains(slot.type) || this->state.non_trivial_dtors[slot.type] == *dtor);
                    this->state.non_trivial_dtors[slot.type] = *dtor;
                }
            }

            co_return;
        }

        auto co_return_value(block_index& current_block, value_index return_value) -> typename CoroutineProvider::template co_type< void >
        {
            auto return_arg_opt = this->local_value_direct_lookup(current_block, "RETURN");

            if (!return_arg_opt.has_value())
            {
                throw std::logic_error("RETURN parameter not found");
            }

            auto return_arg = return_arg_opt.value();

            codegen_invocation_args args;
            args.named["THIS"] = return_arg;
            args.named["OTHER"] = return_value;

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

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_return_statement const& st) -> typename CoroutineProvider::template co_type< void >
        {
            auto return_arg_opt = this->local_value_direct_lookup(current_block, "RETURN");

            if (return_arg_opt.has_value())
            {
                auto return_arg = return_arg_opt.value();

                if (st.expr.has_value())
                {
                    auto expr_index = co_await co_generate_expr(current_block, st.expr.value());

                    co_await co_return_value(current_block, expr_index);
                }

                co_await co_generate_builtin_return(current_block);
            }
            else
            {

                // The only situation where ctx cannot be an instanciation_reference is the constexpr void evaluation
                // which will not have any return type.
                if (!ctx.type_is< instanciation_reference >())
                {
                    assert(ctx == void_type{});
                    this->generate_return(current_block);
                    co_return;
                }

                auto return_type = co_await prv.functanoid_return_type(ctx.get_as< instanciation_reference >());
                assert(typeis< void_type >(return_type));
                this->generate_return(current_block);
            }

            co_return;
        }

        [[nodiscard]] auto co_generate_fblock_statement(block_index& current_block, function_statement const& st) -> typename CoroutineProvider::template co_type< void >
        {
            try
            {
                co_await rpnx::apply_visitor< typename CoroutineProvider::template co_type< void > >(
                    st,
                    [&](auto st) -> typename CoroutineProvider::template co_type< void >
                    {
                        co_return co_await this->co_generate_statement_ovl(current_block, st);
                    });
            }
            catch (compilation_error& err)
            {
                get_location(st);
            }
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

        // Follow chains of jump terminators to find the effective open block
        // where further code can be emitted. If a block ends with a jump to
        // another block, walk the chain until a block without a terminator or
        // with a non-jump terminator is reached.
        block_index resolve_open_block(block_index blk)
        {
            // Guard against degenerate long chains; rely on vector bounds.
            while (this->state.blocks.at(blk).terminator.has_value())
            {
                auto const& term = *this->state.blocks.at(blk).terminator;
                if (term.template type_is< vmir2::jump >())
                {
                    blk = term.template get_as< vmir2::jump >().target;
                    continue;
                }
                break; // Non-jump terminator; cannot append further.
            }
            return blk;
        }

        void generate_jump(block_index& from, block_index& to)
        {
            // If 'from' currently ends in a chain of jumps, append from its open tail.
            from = resolve_open_block(from);
            to = resolve_open_block(to);

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

            assert(!type_is_contextual(func));
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
            assert(!type_is_contextual(func));
            co_await this->co_generate_arg_info(func);
            this->generate_entry_block();
            block_index current_block(0);
            if (!co_await prv.function_builtin(func.temploid))
            {
                if (typeis< submember >(func.temploid.templexoid) && func.temploid.templexoid.template get_as< submember >().name == "CONSTRUCTOR")
                {
                    auto cls = func.temploid.templexoid.get_as< submember >().of;
                    if (cls.template type_is< array_type >())
                    {
                        co_await co_generate_array_ctor_delegates(current_block, func, {});
                    }
                    else
                    {
                        co_await co_generate_struct_ctor_delegates(current_block, func);
                    }
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
                if (typeis< storage >(slot.type) || typeis< aligned_storage >(slot.type))
                {
                    continue;
                }
                auto dtor = co_await prv.class_default_dtor(slot.type);
                if (dtor.has_value())
                {
                    assert(!this->state.non_trivial_dtors.contains(slot.type) || this->state.non_trivial_dtors[slot.type] == *dtor);
                    this->state.non_trivial_dtors[slot.type] = *dtor;
                }
            }

            co_return;
        }

        [[nodiscard]] auto co_generate_struct_ctor_delegates(block_index& bidx, instanciation_reference const& func) -> typename CoroutineProvider::template co_type< void >
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

            co_await co_generate_struct_ctor_delegates(bidx, func, delegates);

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

            if (!val_type.type_is< ptrref_type >())
            {
                throw std::logic_error("Expected a reference type");
            }

            auto vptr = val_type.get_as< ptrref_type >();

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

            if (cls.template type_is< array_type >())
            {
                auto element_type = cls.get_as< array_type >().element_type;
                auto array_size_exp = cls.get_as< array_type >().element_count;
            }

            auto thisidx = this->local_value_direct_lookup(current_block, "THIS");

            QUXLANG_COMPILER_BUG_IF(!thisidx.has_value(), "Expected THIS to be defined");

            auto thisidx_value = thisidx.value();

            auto otheridx = this->local_value_direct_lookup(current_block, "OTHER");
            QUXLANG_COMPILER_BUG_IF(!otheridx.has_value(), "Expected OTHER to be defined");
            auto otheridx_value = otheridx.value();

            this->emit(current_block, vmir2::struct_init_start{.on_value = get_local_index(thisidx_value), .fields = get_invocation_args(fields_args)});

            for (class_field const& fld : fields)
            {
                auto temporary_block = this->generate_subblock(current_block, "copy_ctor_temp_" + fld.name);
                auto after_ctor_block = this->generate_subblock(current_block, "copy_ctor_after_" + fld.name);
                this->generate_jump(current_block, temporary_block);
                auto other_idx_copy = co_await this->co_copy_ref(temporary_block, otheridx_value);
                auto other_field = co_await this->co_generate_dot_access(temporary_block, other_idx_copy, fld.name);
                auto field_type = fld.type;
                assert(!type_is_contextual(field_type));
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
            // this->emit(current_block, vmir2::struct_init_finish{.on_value = get_local_index(thisidx_value)});
        }

        auto co_generate_array_copy_ctor_delegates(block_index& current_block, instanciation_reference const& func) -> typename CoroutineProvider::template co_type< void >
        {
            instanciation_reference const& inst = func;
            auto const& sel = inst.temploid;

            auto functum = sel.templexoid;

            type_symbol cls;

            QUXLANG_COMPILER_BUG_IF(!typeis< submember >(functum), "Expected constructor to be submember");

            cls = as< submember >(functum).of;

            QUXLANG_COMPILER_BUG_IF(!cls.template type_is< array_type >(), "Expected array type in array copy constructor");

            auto element_type = cls.get_as< array_type >().element_type;
            auto array_size_exp = cls.get_as< array_type >().element_count;

            assert(array_size_exp.type_is< expression_numeric_literal >());
            auto array_size = as< expression_numeric_literal >(array_size_exp).value;

            std::uint64_t array_size_uint = 0;

            auto ule = bytemath::detail::string_to_le_raw(array_size);

            bytemath::fixed_int_options opts;
            opts.bits = 64;
            opts.has_sign = false;
            opts.overflow_undefined = true;
            auto [res, ok] = bytemath::unlimited_to_int< std::uint64_t >(opts, ule);

            if (!ok)
            {
                throw std::logic_error("Array size is too large");
            }

            array_initializer_type init_type;
            init_type.count = res;
            init_type.element_type = element_type;

            auto thisidx = this->local_value_direct_lookup(current_block, "THIS");

            QUXLANG_COMPILER_BUG_IF(!thisidx.has_value(), "Expected THIS to be defined");

            auto thisidx_value = thisidx.value();

            auto otheridx = this->local_value_direct_lookup(current_block, "OTHER");
            QUXLANG_COMPILER_BUG_IF(!otheridx.has_value(), "Expected OTHER to be defined");
            auto otheridx_value = otheridx.value();

            auto initiailizer = create_local_value(init_type);

            this->emit(current_block, vmir2::array_init_start{.on_value = get_local_index(thisidx_value), .initializer = get_local_index(initiailizer)});

            auto init_loop_condition_block = this->generate_subblock(current_block, "array_copy_condition_ctor_loop");
            auto init_loop_block = this->generate_subblock(current_block, "array_copy_ctor_loop");
            auto init_loop_done = this->generate_subblock(current_block, "array_copy_ctor_loop_done");

            type_symbol uintptr_type = co_await prv.uintpointer_type({});

            auto remaining_result_bool = this->create_local_value(bool_type{});
            auto element_index = this->create_local_value(uintptr_type);
            auto element = this->create_local_value(element_type);

            this->emit(init_loop_condition_block, vmir2::array_init_more{.initializer = get_local_index(initiailizer), .result = get_local_index(remaining_result_bool)});
            this->generate_branch(init_loop_condition_block, init_loop_block, init_loop_done);
            current_block = init_loop_block;

            this->emit(init_loop_block, vmir2::array_init_element{.initializer = get_local_index(initiailizer), .target = get_local_index(element)});

            auto otherval = (co_await this->co_lookup_symbol(current_block, freebound_identifier{"OTHER"})).value();

            this->emit(init_loop_block, vmir2::array_init_index{.initializer = get_local_index(initiailizer), .result_index = get_local_index(element_index)});
            auto other_element_ref = this->create_local_value(make_cref(element_type));
            this->emit(init_loop_block, vmir2::access_array{.base_index = otherval, .index_index = get_local_index(element_index), .store_index = get_local_index(other_element_ref)});

            auto constructor = submember{.of = element_type, .name = "CONSTRUCTOR"};
            codegen_invocation_args args;
            args.named["THIS"] = element;
            args.named["OTHER"] = other_element_ref;
            co_await this->co_gen_call_functum(init_loop_block, constructor, args);

            this->generate_jump(init_loop_block, init_loop_condition_block);

            current_block = init_loop_done;
            this->emit(init_loop_done, vmir2::array_init_finish{.initializer = get_local_index(initiailizer)});
        }

        auto co_generate_swap_members(block_index& current_block, instanciation_reference const& func) -> typename CoroutineProvider::template co_type< void >
        {
            instanciation_reference const& inst = func;
            auto const& sel = inst.temploid;

            auto functum = sel.templexoid;

            type_symbol cls;

            QUXLANG_COMPILER_BUG_IF(!typeis< submember >(functum), "Expected constructor to be submember");

            cls = as< submember >(functum).of;

            auto const& fields = co_await prv.class_field_list(cls);

            for (class_field const& fld : fields)
            {
                auto temp_block = this->generate_subblock(current_block, "swap_member_temp_" + fld.name);
                auto after_block = this->generate_subblock(current_block, "swap_member_after_" + fld.name);
                this->generate_jump(current_block, temp_block);
                current_block = temp_block;
                auto thisval = (co_await this->co_lookup_symbol(current_block, freebound_identifier{"THIS"})).value();
                auto otherval = (co_await this->co_lookup_symbol(current_block, freebound_identifier{"OTHER"})).value();
                auto this_field = co_await this->co_generate_dot_access(current_block, thisval, fld.name);
                auto other_field = co_await this->co_generate_dot_access(current_block, otherval, fld.name);
                auto field_type = fld.type;
                assert(!type_is_contextual(field_type));
                auto field_swap_functum = submember{.of = field_type, .name = "OPERATOR<->"};
                codegen_invocation_args args;
                args.named["THIS"] = this_field;
                args.named["OTHER"] = other_field;
                co_await this->co_gen_call_functum(current_block, field_swap_functum, args);
                this->generate_jump(current_block, after_block);
                current_block = after_block;
            }
        }

        auto co_generate_move(block_index& current_block, value_index val) -> typename CoroutineProvider::template co_type< value_index >
        {
            type_symbol val_type = this->current_type(current_block, val);
            // This function should convert an mref to tref

            if (!val_type.type_is< ptrref_type >())
            {
                // No-op if the value is not a reference type
                co_return val;
            }

            auto vptr = val_type.get_as< ptrref_type >();

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
                assert(!type_is_contextual(field_type));
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

        [[nodiscard]] auto co_generate_struct_ctor_delegates(block_index& current_block, instanciation_reference const& func, std::vector< delegate > delegates) -> typename CoroutineProvider::template co_type< void >
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

            this->emit(current_block, vmir2::struct_init_start{.on_value = get_local_index(thisidx_value), .fields = get_invocation_args(fields_args)});

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

        [[nodiscard]] auto co_generate_array_ctor_delegates(block_index& current_block, instanciation_reference const& func, std::vector< delegate > delegates) -> typename CoroutineProvider::template co_type< void >
        {
            if (!delegates.empty())
            {
                throw rpnx::unimplemented();
            }
            instanciation_reference const& inst = func;
            auto const& sel = inst.temploid;

            auto functum = sel.templexoid;

            type_symbol cls;

            QUXLANG_COMPILER_BUG_IF(!typeis< submember >(functum), "Expected constructor to be submember");

            cls = as< submember >(functum).of;

            QUXLANG_COMPILER_BUG_IF(!cls.template type_is< array_type >(), "Expected array type in array copy constructor");

            auto element_type = cls.get_as< array_type >().element_type;
            auto array_size_exp = cls.get_as< array_type >().element_count;

            assert(array_size_exp.type_is< expression_numeric_literal >());
            auto array_size = as< expression_numeric_literal >(array_size_exp).value;

            std::uint64_t array_size_uint = 0;

            auto ule = bytemath::detail::string_to_le_raw(array_size);

            bytemath::fixed_int_options opts;
            opts.bits = 64;
            opts.has_sign = false;
            opts.overflow_undefined = true;
            auto [res, ok] = bytemath::unlimited_to_int< std::uint64_t >(opts, ule);

            if (!ok)
            {
                throw std::logic_error("Array size is too large");
            }

            array_initializer_type init_type;
            init_type.count = res;
            init_type.element_type = element_type;

            auto thisidx = this->local_value_direct_lookup(current_block, "THIS");

            QUXLANG_COMPILER_BUG_IF(!thisidx.has_value(), "Expected THIS to be defined");

            auto thisidx_value = thisidx.value();

            auto initiailizer = create_local_value(init_type);

            this->emit(current_block, vmir2::array_init_start{.on_value = get_local_index(thisidx_value), .initializer = get_local_index(initiailizer)});

            auto init_loop_condition_block = this->generate_subblock(current_block, "array_copy_condition_ctor_loop");
            auto init_loop_block = this->generate_subblock(current_block, "array_copy_ctor_loop");
            auto init_loop_done = this->generate_subblock(current_block, "array_copy_ctor_loop_done");

            this->generate_jump(current_block, init_loop_condition_block);

            type_symbol uintptr_type = co_await prv.uintpointer_type({});

            auto remaining_result_bool = this->create_local_value(bool_type{});
            auto element_index = this->create_local_value(uintptr_type);
            auto element = this->create_local_value(element_type);

            this->emit(init_loop_condition_block, vmir2::array_init_more{.initializer = get_local_index(initiailizer), .result = get_local_index(remaining_result_bool)});
            this->generate_branch(remaining_result_bool, init_loop_condition_block, init_loop_block, init_loop_done);
            current_block = init_loop_block;

            this->emit(init_loop_block, vmir2::array_init_element{.initializer = get_local_index(initiailizer), .target = get_local_index(element)});

            this->emit(init_loop_block, vmir2::array_init_index{.initializer = get_local_index(initiailizer), .result = get_local_index(element_index)});

            auto constructor = submember{.of = element_type, .name = "CONSTRUCTOR"};
            codegen_invocation_args args;
            args.named["THIS"] = element;
            co_await this->co_gen_call_functum(init_loop_block, constructor, args);

            this->generate_jump(init_loop_block, init_loop_condition_block);

            current_block = init_loop_done;
            this->emit(init_loop_done, vmir2::array_init_finish{.initializer = get_local_index(initiailizer)});
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_assert_statement const& asrt) -> typename CoroutineProvider::template co_type< void >
        {
            block_index after_block = this->generate_subblock(current_block, "assert_statement_after");
            block_index condition_block = this->generate_subblock(current_block, "if_statement_condition");
            this->generate_jump(current_block, condition_block);
            value_index cond = co_await co_generate_bool_expr(condition_block, asrt.condition);
            auto assert_string = quxlang::to_string(asrt.condition);
            vmir2::assert_instr asrt_instr{.condition = get_local_index(cond), .message = asrt.tagline.value_or("NO_MESSAGE_TAG") + ": " + assert_string, .location = asrt.location};
            this->emit(condition_block, asrt_instr);
            this->generate_jump(condition_block, after_block);
            current_block = after_block;
            co_return;
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_place_statement const& st) -> typename CoroutineProvider::template co_type< void >
        {
            auto storage_ref = co_await co_generate_expr(current_block, st.at);
            auto storage_ref_type = this->current_type(current_block, storage_ref);
            auto storage_type = remove_ref(storage_ref_type);

            if (!is_ref(storage_ref_type) || (!typeis< storage >(storage_type) && !typeis< aligned_storage >(storage_type)))
            {
                this->emit(current_block, vmir2::unimplemented{.message = "PLACE AT on non-storage locations"});
                co_return;
            }

            auto target_type = co_await co_lookup_typeclass(current_block, st.type);
            auto ignored = co_await co_generate_place_expression_impl(current_block, storage_ref, target_type, st.assign_init, st.args);
            (void)ignored;
            co_return;
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_destroy_statement const& st) -> typename CoroutineProvider::template co_type< void >
        {
            auto storage_ref = co_await co_generate_expr(current_block, st.at);
            auto storage_ref_type = this->current_type(current_block, storage_ref);
            auto storage_type = remove_ref(storage_ref_type);

            if (!is_ref(storage_ref_type) || (!typeis< storage >(storage_type) && !typeis< aligned_storage >(storage_type)))
            {
                this->emit(current_block, vmir2::unimplemented{.message = "DESTROY AT on non-storage locations"});
                co_return;
            }

            auto target_type = co_await co_lookup_typeclass(current_block, st.type);
            auto storage_delegate = co_await co_begin_storage_delegate(current_block, storage_ref, target_type, true);

            if (st.args.empty())
            {
                this->emit(current_block, vmir2::destroy{.of = get_local_index(storage_delegate)});
            }
            else
            {
                auto destructor = submember{.of = target_type, .name = "DESTRUCTOR"};
                codegen_invocation_args dtor_args;
                dtor_args.named["THIS"] = storage_delegate;

                for (auto const& arg : st.args)
                {
                    auto arg_val = co_await co_generate_expr(current_block, arg.value);
                    if (arg.name.has_value())
                    {
                        dtor_args.named[*arg.name] = arg_val;
                    }
                    else
                    {
                        dtor_args.positional.push_back(arg_val);
                    }
                }

                co_await co_gen_call_functum(current_block, destructor, dtor_args);
            }
            co_return;
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_runtime_statement const& st) -> typename CoroutineProvider::template co_type< void >
        {
            // Generate blocks similar to an if-statement, but condition is based on RT_CE
            block_index after_block = this->generate_subblock(current_block, "runtime_statement_after");
            block_index condition_block = this->generate_subblock(current_block, "runtime_statement_condition");
            block_index then_block = this->generate_subblock(current_block, "runtime_then");

            this->generate_jump(current_block, condition_block);

            // Emit RT_CE into a temporary bool
            auto rt_flag = this->create_local_value(bool_type{});
            this->emit(condition_block, vmir2::runtime_ce{.target = get_local_index(rt_flag)});

            value_index branch_cond = rt_flag;

            if (st.condition == runtime_condition::NATIVE)
            {
                // For NATIVE, we want true when not constexpr -> negate the RT_CE flag
                auto native_flag = this->create_local_value(bool_type{});
                this->emit(condition_block, vmir2::to_bool_not{.from = get_local_index(rt_flag), .to = get_local_index(native_flag)});
                branch_cond = native_flag;
            }
            // For CONSTEXPR, branch_cond is the RT_CE flag as-is

            if (!st.else_block.has_value())
            {
                this->generate_branch(branch_cond, condition_block, then_block, after_block);

                // Then
                co_await co_generate_function_block(then_block, st.then_block, "runtime_then");
                this->generate_jump(then_block, after_block);
            }
            else
            {
                block_index else_block = this->generate_subblock(current_block, "runtime_else");
                this->generate_branch(branch_cond, condition_block, then_block, else_block);

                // Then
                co_await co_generate_function_block(then_block, st.then_block, "runtime_then");
                this->generate_jump(then_block, after_block);

                // Else
                co_await co_generate_function_block(else_block, *st.else_block, "runtime_else");
                this->generate_jump(else_block, after_block);
            }

            current_block = after_block;

            co_return;
        }
    };

} // namespace quxlang

#endif // RPNX_QUXLANG_CO_VMIR_EXPRESSION_EMITTER_HEADER
