// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com
// Agents: MUST READ: docs/vmir2_lowering.md BEFORE EDITING
#ifndef QUXLANG_CO_VMIR_GENERATOR2_HEADER_GUARD
#define QUXLANG_CO_VMIR_GENERATOR2_HEADER_GUARD

#include "quxlang/ast2/ast2_entity.hpp"
#include "quxlang/bytemath.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/class_field_declaration.hpp"
#include "quxlang/data/class_layout.hpp"
#include "quxlang/data/codegen_types.hpp"
#include "quxlang/data/compilation_result.hpp"
#include "quxlang/data/contextual_type_reference.hpp"
#include "quxlang/data/lambda_types.hpp"
#include <quxlang/data/basic_types.hpp>
#include "quxlang/data/machine.hpp"
#include "quxlang/data/type_placement_info.hpp"
#include "quxlang/exception.hpp"
#include "quxlang/fixed_bytemath.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/parsers/parse_int.hpp"
#include "quxlang/queries/class_default_ctor.hpp"
#include "quxlang/queries/class_default_dtor.hpp"
#include "quxlang/queries/class_field_list.hpp"
#include "quxlang/queries/class_layout.hpp"
#include "quxlang/queries/constexpr_bool.hpp"
#include "quxlang/queries/constexpr_eval_v3.hpp"
#include "quxlang/queries/constexpr_u64.hpp"
#include "quxlang/queries/ensig_argument_initialize.hpp"
#include "quxlang/queries/enum_info.hpp"
#include "quxlang/queries/flagset_info.hpp"
#include "quxlang/queries/functanoid_deduced_return_type.hpp"
#include "quxlang/queries/functanoid_return_type.hpp"
#include "quxlang/queries/functanoid_sigtype.hpp"
#include "quxlang/queries/function_builtin.hpp"
#include "quxlang/queries/function_declaration.hpp"
#include "quxlang/queries/function_ensig_init_with.hpp"
#include "quxlang/queries/function_pack_info.hpp"
#include "quxlang/queries/function_param_names.hpp"
#include "quxlang/queries/function_primitive.hpp"
#include "quxlang/queries/functum_overloads.hpp"
#include "quxlang/queries/functum_select_function.hpp"
#include "quxlang/queries/global_is_string_static.hpp"
#include "quxlang/queries/global_is_serialoid_static.hpp"
#include "quxlang/queries/implementation_function_map.hpp"
#include "quxlang/queries/implementation_interface_type.hpp"
#include "quxlang/queries/implicitly_convertible_to.hpp"
#include "quxlang/queries/instanciation.hpp"
#include "quxlang/queries/instanciation_concrete_params.hpp"
#include "quxlang/queries/interface_slot_list.hpp"
#include "quxlang/queries/lambda_capture_set.hpp"
#include "quxlang/queries/lambda_environment.hpp"
#include "quxlang/queries/lambda_operator.hpp"
#include "quxlang/queries/lambda_possible_captures.hpp"
#include "quxlang/queries/lookup.hpp"
#include "quxlang/queries/module_options_map.hpp"
#include "quxlang/queries/serialoid_static_value.hpp"
#include "quxlang/queries/string_static_value.hpp"
#include "quxlang/queries/symboid.hpp"
#include "quxlang/queries/symbol_type.hpp"
#include "quxlang/queries/temploid_formal_ensig.hpp"
#include "quxlang/queries/type_is_serialoid.hpp"
#include "quxlang/queries/type_is_stringlike.hpp"
#include "quxlang/queries/type_placement_info.hpp"
#include "quxlang/queries/uintpointer_type.hpp"
#include "quxlang/queries/variable_type.hpp"
#include "quxlang/queries/vm_procedure3.hpp"
#include "quxlang/vmir2/assembler.hpp"
#include "quxlang/vmir2/state_engine.hpp"
#include "quxlang/vmir2/vmir2.hpp"
#include "rpnx/querygraph/querygraph.hpp"

#include <algorithm>
#include <assert.h>
#include <limits>
#include <quxlang/macros.hpp>
#include <set>
#include <string_view>
#include <utility>

namespace quxlang
{

    template < typename T >
    concept codegen_co_base = requires {
        { std::declval< typename T::template cosubroutine< quxlang::type_symbol > >() };
        { std::declval< typename T::template cosubroutine< quxlang::vmir2::local_index > >() };
    };

    template < codegen_co_base CoroutineBaseType >
    class co_vmir_generator2
    {

        // The following structs are basically integer types but they can't be interconverted with each other.
        // They are used to represent different indices in the code generation this->state.

      private:
        using block_index = vmir2::block_index;
        using local_index = vmir2::local_index;

        struct codegen_literal;
        struct codegen_local;
        struct codegen_argument;
        struct codegen_pack;
        struct codegen_static;
        struct codegen_static_scope;

        /// Mutability policy used when generating a temporary constexpr evaluation routine.
        enum class static_eval_access : std::uint8_t
        {
            /// STATIC_VAR bindings are mutable and returned through their nonzero result IDs.
            mutable_view,
            /// All visible statics are exposed as read-only snapshots with no mutation results.
            readonly_view,
        };

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

        /// Expanded local parameter metadata for one source-level positional pack.
        struct codegen_pack
        {
            /// Expanded local values for each concrete positional argument captured by this pack.
            std::vector< value_index > values;
            /// Concrete accepted parameter types for each captured positional argument.
            std::vector< type_symbol > types;

            RPNX_MEMBER_METADATA(codegen_pack, values, types);
        };

        using codegen_value = rpnx::variant< codegen_binding, codegen_literal, codegen_local >;

        struct codegen_static
        {
            /// Declared static object type.
            type_symbol type;
            /// Current generation-time antestatal value.
            constexpr_value value;
            /// Result ID for mutable constexpr updates, or nullopt for read-only statics.
            std::optional< std::uint64_t > mutation_result_id;

            RPNX_MEMBER_METADATA(codegen_static, type, value, mutation_result_id);
        };

        struct codegen_static_scope
        {
            /// Visible static names for one generated function block, mapped to static generations.
            std::map< std::string, static_local_ref > bindings;

            RPNX_MEMBER_METADATA(codegen_static_scope, bindings);
        };

        struct loop_control_targets
        {
            std::optional< std::string > label_name;
            block_index break_target;
            block_index continue_target;

            RPNX_MEMBER_METADATA(loop_control_targets, label_name, break_target, continue_target);
        };

        struct break_control_targets
        {
            std::string label_name;
            block_index break_target;

            RPNX_MEMBER_METADATA(break_control_targets, label_name, break_target);
        };

        struct goto_label_target
        {
            block_index target;
            bool declared = false;
            std::optional< source_location > location;

            RPNX_MEMBER_METADATA(goto_label_target, target, declared, location);
        };

        struct pending_goto_fixup
        {
            block_index source;
            std::string target;
            std::optional< source_location > location;

            RPNX_MEMBER_METADATA(pending_goto_fixup, source, target, location);
        };

        struct lambda_capture_selection
        {
            std::string name;
            lambda_capture_mode mode = lambda_capture_mode::reference;
            type_symbol field_type;
        };

        struct lambda_dry_run_result
        {
            std::vector< lambda_capture_selection > captures;
            lambda_environment environment;
        };

        struct lambda_dry_static_context
        {
            std::map< std::string, scoped_definition_v3 > scoped_definitions;
            std::map< static_local_ref, codegen_static > statics;
            std::vector< codegen_static_scope > static_scopes;
        };

        struct lambda_capture_analysis_state
        {
            std::map< std::string, lambda_possible_capture > possible_captures;
            std::map< std::string, lambda_capture_mode > explicit_captures;
            bool has_explicit_capture_list = false;
            std::map< std::string, type_symbol > local_types;
            std::vector< lambda_capture_selection > captures;
            std::map< std::string, std::size_t > capture_indices;
            lambda_dry_static_context static_context;
        };

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
            std::optional< type_symbol > declared_return_type;
            std::optional< type_symbol > deduced_return_type;
            std::optional< source_location > current_source_location;

            /// Scoped typedef and static definitions visible to constexpr routine generation.
            std::map< std::string, scoped_definition_v3 > scoped_definitions;
            /// Visible positional variadic packs for the current function body.
            std::map< std::string, codegen_pack > packs;
            /// Static objects tracked by stable static-local symbol.
            std::map< static_local_ref, codegen_static > statics;

            /// Stack of generated block scopes that own visible static names.
            std::vector< codegen_static_scope > static_scopes;
            /// Stack of runtime loop targets for BREAK and CONTINUE statements.
            std::vector< loop_control_targets > loop_controls;
            /// Stack of labeled runtime blocks and loops for labeled BREAK statements.
            std::vector< break_control_targets > break_controls;
            /// Point-label targets used by GOTO statements.
            std::map< std::string, goto_label_target > goto_labels;
            /// GOTO statements whose point label has not been declared yet.
            std::map< std::string, std::vector< pending_goto_fixup > > pending_gotos;
            /// Next nonzero result ID assigned to a STATIC_VAR binding.
            std::uint64_t next_static_result_id = 1;
            /// Next declaration generation to assign for each static local name.
            std::map< std::string, std::uint64_t > next_static_generation;
            /// Next immutable runtime-read snapshot ID to assign.
            std::uint64_t next_static_snapshot_id = 1;
            /// Immutable snapshot localdata emitted into the current routine.
            std::map< static_snapshot_ref, vmir2::localdata_entry > static_snapshots;
            /// Next lambda closure index in encounter order for this generated routine.
            std::size_t next_lambda_index = 0;
        };

        codegen_state state;
        type_symbol ctx;
        output_info machine_info;

        class source_location_scope
        {
          public:
            source_location_scope(co_vmir_generator2& owner, std::optional< source_location > location) : owner(owner), previous(owner.state.current_source_location)
            {
                if (location.has_value())
                {
                    owner.state.current_source_location = location;
                }
            }

            source_location_scope(source_location_scope const&) = delete;
            source_location_scope& operator=(source_location_scope const&) = delete;

            ~source_location_scope()
            {
                owner.state.current_source_location = previous;
            }

          private:
            co_vmir_generator2& owner;
            std::optional< source_location > previous;
        };

        auto scoped_source_location(std::optional< source_location > location) -> source_location_scope
        {
            return source_location_scope(*this, location);
        }

        class declaration_context_scope
        {
          public:
            declaration_context_scope(co_vmir_generator2& owner, block_index lookup_block, type_symbol declaration_context) :
                owner(owner),
                lookup_block(lookup_block),
                previous_context(owner.ctx),
                previous_block_lookups(std::move(owner.state.blocks.at(lookup_block).lookup_values)),
                previous_top_level_lookups(std::move(owner.state.top_level_lookups)),
                previous_top_level_lookups_weak(std::move(owner.state.top_level_lookups_weak)),
                previous_packs(std::move(owner.state.packs)),
                previous_scoped_definitions(owner.state.scoped_definitions),
                previous_statics(owner.state.statics),
                previous_static_scopes(owner.state.static_scopes)
            {
                owner.ctx = std::move(declaration_context);
                owner.state.blocks.at(lookup_block).lookup_values.clear();
                owner.state.top_level_lookups.clear();
                owner.state.top_level_lookups_weak.clear();
                owner.state.packs.clear();
                owner.state.static_scopes.clear();
            }

            declaration_context_scope(declaration_context_scope const&) = delete;
            declaration_context_scope& operator=(declaration_context_scope const&) = delete;

            ~declaration_context_scope()
            {
                owner.ctx = std::move(previous_context);
                owner.state.blocks.at(lookup_block).lookup_values = std::move(previous_block_lookups);
                owner.state.top_level_lookups = std::move(previous_top_level_lookups);
                owner.state.top_level_lookups_weak = std::move(previous_top_level_lookups_weak);
                owner.state.packs = std::move(previous_packs);
                owner.state.scoped_definitions = std::move(previous_scoped_definitions);
                owner.state.statics = std::move(previous_statics);
                owner.state.static_scopes = std::move(previous_static_scopes);
            }

          private:
            co_vmir_generator2& owner;
            block_index lookup_block;
            type_symbol previous_context;
            std::map< std::string, value_index > previous_block_lookups;
            std::map< std::string, value_index > previous_top_level_lookups;
            std::map< std::string, value_index > previous_top_level_lookups_weak;
            std::map< std::string, codegen_pack > previous_packs;
            std::map< std::string, scoped_definition_v3 > previous_scoped_definitions;
            std::map< static_local_ref, codegen_static > previous_statics;
            std::vector< codegen_static_scope > previous_static_scopes;
        };

        auto scoped_declaration_context(block_index lookup_block, type_symbol declaration_context) -> declaration_context_scope
        {
            return declaration_context_scope(*this, lookup_block, std::move(declaration_context));
        }

        void apply_current_source_location(vmir2::vm_instruction& instruction)
        {
            if (!this->state.current_source_location.has_value() || vmir2::get_location(instruction).has_value())
            {
                return;
            }

            auto location = this->state.current_source_location;
            rpnx::apply_visitor< void >(instruction, [&](auto& item) { item.location = location; });
        }

        void apply_current_source_location(vmir2::vm_terminator& terminator)
        {
            if (!this->state.current_source_location.has_value() || vmir2::get_location(terminator).has_value())
            {
                return;
            }

            auto location = this->state.current_source_location;
            rpnx::apply_visitor< void >(terminator, [&](auto& item) { item.location = location; });
        }

        void set_terminator(block_index idx, vmir2::vm_terminator terminator)
        {
            if (this->state.blocks.at(idx).terminator.has_value())
            {
                throw compiler_bug("Attempted to replace block terminator");
            }
            this->apply_current_source_location(terminator);
            this->state.blocks.at(idx).terminator = std::move(terminator);
        }

        template < typename T >
        using co_type = typename CoroutineBaseType::template cosubroutine< T >;
        using handler_spec = typename CoroutineBaseType::spec_type;

      public:
        co_vmir_generator2(output_info machine_info, type_symbol ctx) : ctx(std::move(ctx)), machine_info(std::move(machine_info))
        {
        }

        auto set_scoped_definitions(std::map< std::string, rpnx::variant< constexpr_result, type_symbol > > defs) -> void
        {
            this->state.scoped_definitions.clear();
            for (auto& [name, def] : defs)
            {
                if (def.template type_is< type_symbol >())
                {
                    this->state.scoped_definitions[std::move(name)] = scoped_typedef{.type = std::move(def.template get_as< type_symbol >())};
                    continue;
                }
                throw rpnx::unimplemented();
            }
        }

        /// Configures scoped typedef and static definitions for constexpr v3 routine generation.
        auto set_scoped_definitions_v3(std::map< std::string, scoped_definition_v3 > defs) -> void
        {
            this->state.scoped_definitions = std::move(defs);
        }

        /// Configures function-local static localdata visible while generating a constexpr routine.
        auto set_static_eval_context(std::map< static_local_ref, constexpr_static > inputs, std::map< std::string, static_local_ref > scoped_symbols, bool emit_results, bool) -> void
        {
            this->state.statics.clear();
            for (auto& [symbol, input] : inputs)
            {
                if (!emit_results)
                {
                    input.mutation_result_id.reset();
                }
                this->state.statics[std::move(symbol)] = codegen_static{.type = std::move(input.type), .value = std::move(input.value), .mutation_result_id = input.mutation_result_id};
            }
            for (auto& [name, symbol] : scoped_symbols)
            {
                this->state.scoped_definitions[std::move(name)] = scoped_static{.symbol = std::move(symbol)};
            }
        }

        /// Configures function-local static localdata visible while generating a constexpr v3 routine.
        auto set_static_eval_context_v3(std::map< static_local_ref, constexpr_static > inputs) -> void
        {
            this->state.statics.clear();
            for (auto& [symbol, input] : inputs)
            {
                this->state.statics[std::move(symbol)] = codegen_static{.type = std::move(input.type), .value = std::move(input.value), .mutation_result_id = input.mutation_result_id};
            }
        }

        auto co_generate_constexpr_eval(expression expr, type_symbol type) -> co_type< vmir2::functanoid_routine3 >
        {
            assert(this->state.blocks.empty());
            this->state.blocks.push_back(codegen_block{});
            auto current_block = block_index(0);
            auto location_scope = this->scoped_source_location(get_location(expr));
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

        /// Emits code that creates a constexpr output proxy for result_id and calls value_type.SERIALIZE with result_val as THIS.
        auto co_emit_constexpr_serialoid_result(block_index& current_block, value_index result_val, type_symbol value_type, std::uint64_t result_id) -> co_type< void >
        {
            auto proxy = this->create_local_value(constexpr_proxy{});
            this->emit(current_block, vmir2::constexpr_make_proxy{
                                          .target = get_local_index(proxy),
                                          .result_id = result_id,
                                      });
            auto proxy_ref = this->create_reference(current_block, proxy, make_mref(type_symbol(constexpr_proxy{})));
            auto serialize_functum = submember{.of = std::move(value_type), .name = "SERIALIZE"};
            co_await this->co_gen_call_functum(current_block, serialize_functum, codegen_invocation_args{.named = {{"THIS", result_val}, {"OUTPUT_ITERATOR", proxy_ref}}});
        }

        /// Encodes an unsigned integer using UINTANY: continuation bytes carry seven payload bits and store remaining / 128 minus one.
        auto encode_uintany(std::uint64_t value) -> std::vector< std::byte >
        {
            std::vector< std::byte > result;
            while (value >= 128)
            {
                result.push_back(static_cast< std::byte >((value % 128) | 128));
                value = (value / 128) - 1;
            }
            result.push_back(static_cast< std::byte >(value));
            return result;
        }

        /// Creates a constexpr output proxy local and returns a mutable reference to it for the requested result ID.
        auto create_constexpr_proxy_ref(block_index& current_block, std::uint64_t result_id) -> value_index
        {
            auto proxy = this->create_local_value(constexpr_proxy{});
            this->emit(current_block, vmir2::constexpr_make_proxy{
                                          .target = get_local_index(proxy),
                                          .result_id = result_id,
                                      });
            return this->create_reference(current_block, proxy, make_mref(type_symbol(constexpr_proxy{})));
        }

        /// Emits one byte through a constexpr proxy reference while preserving the caller's reference for later writes.
        auto co_emit_proxy_output_byte(block_index& current_block, value_index proxy_ref, value_index byte_value) -> co_type< void >
        {
            auto consumed_proxy_ref = this->copy_refernece_internal(current_block, proxy_ref);
            co_await co_generate_binary(current_block, ":=", consumed_proxy_ref, byte_value);
            co_return;
        }

        /// Emits a string literal result as UINTANY byte length followed by the literal's bytes.
        auto co_emit_constexpr_string_literal_result(block_index& current_block, value_index literal, std::uint64_t result_id) -> co_type< void >
        {
            auto const& literal_slot = this->state.genvalues.at(static_cast< std::uint64_t >(literal));
            if (!literal_slot.template type_is< codegen_literal >())
            {
                throw compiler_bug("string literal result is not a codegen literal");
            }
            auto const& literal_value = literal_slot.template get_as< codegen_literal >().value;

            auto proxy_ref = create_constexpr_proxy_ref(current_block, result_id);
            auto encoded_length = encode_uintany(literal_value.size());
            for (auto byte : encoded_length)
            {
                auto byte_value = create_small_uint_value(current_block, std::to_integer< std::uint8_t >(byte), byte_type{});
                co_await co_emit_proxy_output_byte(current_block, proxy_ref, byte_value);
            }
            for (auto byte : literal_value)
            {
                auto byte_value = create_small_uint_value(current_block, std::to_integer< std::uint8_t >(byte), byte_type{});
                co_await co_emit_proxy_output_byte(current_block, proxy_ref, byte_value);
            }
            co_return;
        }

        /// Emits a STRING_CONSTANT result by calling BEGIN and END, counting the bytes, writing the count as UINTANY, and then writing each byte.
        auto co_emit_constexpr_string_constant_result(block_index& current_block, value_index string_value, std::uint64_t result_id) -> co_type< void >
        {
            auto string_type = remove_ref(this->current_type(current_block, string_value));
            if (!typeis< readonly_constant >(string_type) || as< readonly_constant >(string_type).kind != constant_kind::string)
            {
                throw compiler_bug("constexpr string constant result requires STRING_CONSTANT input");
            }

            /// Returns a fresh reference suitable for passing as THIS to STRING_CONSTANT methods.
            auto create_string_this_ref = [&]() -> value_index
            {
                auto current_string_type = this->current_type(current_block, string_value);
                if (is_ref(current_string_type))
                {
                    return this->copy_refernece_internal(current_block, string_value);
                }
                return this->create_reference(current_block, string_value, make_cref(string_type));
            };

            auto begin_functum = submember{.of = string_type, .name = "BEGIN"};
            auto end_functum = submember{.of = string_type, .name = "END"};
            auto begin_iter = co_await this->co_gen_call_functum(current_block, begin_functum, codegen_invocation_args{.named = {{"THIS", create_string_this_ref()}}});
            auto end_iter = co_await this->co_gen_call_functum(current_block, end_functum, codegen_invocation_args{.named = {{"THIS", create_string_this_ref()}}});
            auto iter_type = this->current_type(current_block, begin_iter);
            auto uintptr_type = co_await rpnx::querygraph::request< uintpointer_type_query >({});

            auto count = load_zero_value(current_block, uintptr_type);
            auto count_iter = co_await co_construct_copy(current_block, begin_iter, iter_type);

            auto count_condition_block = this->generate_subblock(current_block, "constexpr_string_count_condition");
            auto count_body_block = this->generate_subblock(current_block, "constexpr_string_count_body");
            auto emit_length_block = this->generate_subblock(current_block, "constexpr_string_emit_length");
            this->generate_jump(current_block, count_condition_block);

            auto count_iter_value = co_await co_construct_copy(count_condition_block, count_iter, iter_type);
            auto count_end_value = co_await co_construct_copy(count_condition_block, end_iter, iter_type);
            auto count_has_more = co_await co_generate_binary(count_condition_block, "<", count_iter_value, count_end_value);
            this->generate_branch(count_has_more, count_condition_block, count_body_block, emit_length_block);

            auto count_ref = this->create_reference(count_body_block, count, make_mref(uintptr_type));
            auto one = create_small_uint_value(count_body_block, 1, uintptr_type);
            auto old_count = co_await co_construct_copy(count_body_block, count, uintptr_type);
            auto next_count = co_await co_generate_binary(count_body_block, "+", old_count, one);
            co_await co_store_local_value(count_body_block, count, next_count, uintptr_type);
            (void)count_ref;
            auto count_iter_ref = this->create_reference(count_body_block, count_iter, make_mref(iter_type));
            co_await co_generate_unary_postfix(count_body_block, "++", count_iter_ref);
            this->generate_jump(count_body_block, count_condition_block);

            current_block = emit_length_block;
            auto proxy_ref = create_constexpr_proxy_ref(current_block, result_id);
            auto count_cref = this->create_reference(current_block, count, make_cref(uintptr_type));
            co_await this->co_gen_call_functum(current_block, builtin_symbol{"SERIALIZE_UINTANY"}, codegen_invocation_args{.named = {{"VALUE", count_cref}, {"OUTPUT_ITERATOR", proxy_ref}}});

            auto emit_iter = co_await co_construct_copy(current_block, begin_iter, iter_type);
            auto emit_condition_block = this->generate_subblock(current_block, "constexpr_string_emit_condition");
            auto emit_body_block = this->generate_subblock(current_block, "constexpr_string_emit_body");
            auto done_block = this->generate_subblock(current_block, "constexpr_string_emit_done");
            this->generate_jump(current_block, emit_condition_block);

            auto emit_iter_value = co_await co_construct_copy(emit_condition_block, emit_iter, iter_type);
            auto emit_end_value = co_await co_construct_copy(emit_condition_block, end_iter, iter_type);
            auto emit_has_more = co_await co_generate_binary(emit_condition_block, "<", emit_iter_value, emit_end_value);
            this->generate_branch(emit_has_more, emit_condition_block, emit_body_block, done_block);

            auto emit_iter_ref = this->create_reference(emit_body_block, emit_iter, make_mref(iter_type));
            auto current_byte_ref = co_await co_generate_unary_postfix(emit_body_block, "->", co_await co_generate_unary_postfix(emit_body_block, "++", emit_iter_ref));
            auto current_byte = load_reference_value(emit_body_block, current_byte_ref, byte_type{});
            co_await co_emit_proxy_output_byte(emit_body_block, proxy_ref, current_byte);
            this->generate_jump(emit_body_block, emit_condition_block);

            current_block = done_block;
            co_return;
        }

        /// Emits the constexpr string result for a literal, STRING_CONSTANT, or STRINGLIKE value into the selected result buffer.
        auto co_emit_constexpr_string_result(block_index& current_block, value_index result_val, std::uint64_t result_id) -> co_type< void >
        {
            auto result_type = this->current_type(current_block, result_val);
            auto result_value_type = remove_ref(result_type);

            if (typeis< string_literal_reference >(result_type))
            {
                co_await co_emit_constexpr_string_literal_result(current_block, result_val, result_id);
                co_return;
            }

            if (typeis< readonly_constant >(result_value_type) && as< readonly_constant >(result_value_type).kind == constant_kind::string)
            {
                co_await co_emit_constexpr_string_constant_result(current_block, result_val, result_id);
                co_return;
            }

            if (co_await rpnx::querygraph::request< type_is_stringlike_query >(result_value_type))
            {
                co_await this->co_emit_constexpr_serialoid_result(current_block, result_val, result_value_type, result_id);
                co_return;
            }

            throw semantic_compilation_error("constexpr string evaluation requires STRING_CONSTANT or STRINGLIKE input, got: " + quxlang::to_string(result_type));
        }

        /// Generates a constexpr v3 routine that can return primary and static mutation results.
        auto co_generate_constexpr_eval_v3(expression expr, std::optional< type_symbol > expected_result_type) -> co_type< constexpr_routine_v3_result >
        {
            assert(this->state.blocks.empty());
            this->state.blocks.push_back(codegen_block{});
            auto current_block = block_index(0);
            auto location_scope = this->scoped_source_location(get_location(expr));
            std::string expr_str = to_string(expr);
            std::optional< type_symbol > deduced_type;
            std::optional< type_symbol > type_binding_result;
            if (expected_result_type.has_value())
            {
                value_index result_val;
                if (typeis< readonly_constant >(*expected_result_type) && as< readonly_constant >(*expected_result_type).kind == constant_kind::string)
                {
                    result_val = co_await this->co_generate_expr(current_block, expr);
                    co_await this->co_emit_constexpr_string_result(current_block, result_val, constexpr_primary_result_id);
                }
                else if (typeis< auto_temploidic >(*expected_result_type))
                {
                    result_val = co_await this->co_generate_expr(current_block, expr);
                    deduced_type = this->current_type(current_block, result_val);
                    assert(current_block == block_index(0) || this->state.blocks.at(0).terminator.has_value());
                    assert(result_val != value_index(0));
                    vmir2::constexpr_set_result2 csr;
                    auto result_type = this->current_type(current_block, result_val);
                    if (co_await rpnx::querygraph::request< type_is_serialoid_query >(result_type))
                    {
                        co_await this->co_emit_constexpr_serialoid_result(current_block, result_val, result_type, constexpr_primary_result_id);
                    }
                    else
                    {
                        csr.target = get_local_index(result_val);
                        csr.result_id = constexpr_primary_result_id;
                        this->emit(current_block, csr);
                    }
                }
                else
                {
                    result_val = co_await this->co_generate_typed_expr(current_block, expr, *expected_result_type);
                    assert(current_block == block_index(0) || this->state.blocks.at(0).terminator.has_value());
                    assert(result_val != value_index(0));
                    vmir2::constexpr_set_result2 csr;
                    assert(this->current_type(current_block, result_val) == *expected_result_type);
                    auto result_type = this->current_type(current_block, result_val);
                    if (co_await rpnx::querygraph::request< type_is_serialoid_query >(result_type))
                    {
                        co_await this->co_emit_constexpr_serialoid_result(current_block, result_val, result_type, constexpr_primary_result_id);
                    }
                    else
                    {
                        csr.target = get_local_index(result_val);
                        csr.result_id = constexpr_primary_result_id;
                        this->emit(current_block, csr);
                    }
                }
            }
            else
            {
                auto result_val = co_await this->co_generate_void_expr(current_block, expr);
                auto result_type = this->current_type(current_block, result_val);
                if (typeis< attached_type_reference >(result_type))
                {
                    auto const& attached = as< attached_type_reference >(result_type);
                    auto attached_symbol_kind = co_await rpnx::querygraph::request< symbol_type_query >(attached.attached_symbol);
                    if (attached_symbol_kind == symbol_kind::functum || attached_symbol_kind == symbol_kind::funtanoid)
                    {
                        type_binding_result = result_type;
                    }
                    else if (typeis< void_type >(attached.carrying_type))
                    {
                        type_binding_result = attached.attached_symbol;
                    }
                }
            }

            for (auto const& [symbol, input] : this->state.statics)
            {
                if (!input.mutation_result_id.has_value())
                {
                    continue;
                }
                auto ref = this->create_local_value(make_mref(input.type));
                this->emit(current_block, vmir2::get_antestatal_ref{.symbol = type_symbol(symbol), .target_ref = get_local_index(ref)});
                if (co_await rpnx::querygraph::request< type_is_serialoid_query >(input.type))
                {
                    co_await this->co_emit_constexpr_serialoid_result(current_block, ref, input.type, *input.mutation_result_id);
                }
                else
                {
                    this->emit(current_block, vmir2::constexpr_set_result2{
                                                  .target = get_local_index(ref),
                                                  .result_id = *input.mutation_result_id,
                                                  .target_mode = vmir2::constexpr_result_target_mode::referenced_object,
                                              });
                }
            }

            this->generate_return(current_block);

            co_await co_generate_dtor_references();

            co_return constexpr_routine_v3_result{.routine = get_result(), .deduced_type = std::move(deduced_type), .type_binding_result = std::move(type_binding_result)};
        }

        /// Generates a legacy antestatal constexpr routine by adapting to the v3 generator.
        auto co_generate_constexpr_eval_antestatal(expression expr, type_symbol type) -> co_type< vmir2::functanoid_routine3 >
        {
            auto result = co_await this->co_generate_constexpr_eval_v3(std::move(expr), std::move(type));
            co_return std::move(result.routine);
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
            if (state.genvalues.at(idx).template type_is< codegen_binding >())
            {
                value_index bound_value = state.genvalues.at(idx).template get_as< codegen_binding >().bound_value;
                if (bound_value == value_index(0))
                {
                    return true;
                }
                return value_alive(bidx, bound_value);
            }
            return local_alive(bidx, get_local_index(idx));
        }

        auto co_generate_expr(block_index& bidx, expression const& expr) -> co_type< value_index >
        {
            auto location_scope = this->scoped_source_location(get_location(expr));
            assert(bidx == block_index(0) || this->state.blocks.at(0).terminator.has_value());
            auto result = co_await rpnx::apply_visitor< co_type< value_index > >(expr,
                                                                                 [&](auto&& val)
                                                                                 {
                                                                                     return co_generate(bidx, std::forward< decltype(val) >(val));
                                                                                 });
            std::string expr_str = to_string(expr);
            assert(bidx == block_index(0) || this->state.blocks.at(0).terminator.has_value());
            co_return result;
        }

        auto co_gen_argument_adaptation(block_index& bidx, value_index val, type_symbol target_type, allowed_adaptations adaptations) -> co_type< value_index >
        {
            auto value_type = this->current_type(bidx, val);

            if (value_type == target_type)
            {
                assert(val != value_index(0));
                co_return val;
            }

            if (target_type.type_is< nvalue_slot >() && value_type == target_type.get_as< nvalue_slot >().target)
            {
                assert(val != value_index(0));
                co_return val;
            }

            if (target_type.type_is< dvalue_slot >() && value_type == target_type.get_as< dvalue_slot >().target)
            {
                assert(val != value_index(0));
                co_return val;
            }

            if (typeis< attached_type_reference >(value_type) || typeis< attached_type_reference >(target_type))
            {
                throw semantic_compilation_error("Cannot adapt attached binding " + to_string(value_type) + " to " + to_string(target_type));
            }

            if (is_ref(target_type) && remove_ref(value_type) != remove_ref(target_type))
            {
                auto target_value_type = remove_ref(target_type);
                auto converted_value = co_await co_gen_construct_with_target_type(bidx, val, target_value_type, adaptations);
                co_return create_reference(bidx, converted_value, target_type);
            }

            co_return co_await co_gen_construct_with_target_type(bidx, val, target_type, adaptations);
        }

        auto nested_constructor_adaptations(allowed_adaptations adaptations) -> allowed_adaptations
        {
            switch (adaptations)
            {
            case allowed_adaptations::destination_rebinding:
            case allowed_adaptations::class_conversions:
                return allowed_adaptations::source_rebinding;
            case allowed_adaptations::source_rebinding:
                // Objectization is lowered through a constructor call, but that
                // constructor still needs the same one-step source rebinding
                // budget to bind copy-like parameters such as CONST& T.
                return allowed_adaptations::source_rebinding;
            case allowed_adaptations::none:
                return allowed_adaptations::none;
            default:
                throw compiler_bug("Invalid constructor adaptation ceiling");
            }
        }

        /**
         * Returns the positional pack index for an overload candidate, if present.
         */
        auto call_candidate_positional_pack_index(temploid_ensig const& ensig) -> std::optional< std::size_t >
        {
            std::optional< std::size_t > result;
            for (std::size_t i = 0; i < ensig.interface.positional.size(); i++)
            {
                if (!ensig.interface.positional.at(i).is_pack)
                {
                    continue;
                }
                if (result.has_value())
                {
                    throw compiler_bug("overload candidate has more than one positional pack");
                }
                result = i;
            }
            return result;
        }

        /**
         * Returns the formal positional parameter used for a concrete argument index.
         */
        auto call_candidate_positional_formal_for(temploid_ensig const& ensig, std::size_t argument_index) -> argif const&
        {
            std::optional< std::size_t > const pack_index = this->call_candidate_positional_pack_index(ensig);
            if (pack_index.has_value() && argument_index >= *pack_index)
            {
                return ensig.interface.positional.at(*pack_index);
            }
            return ensig.interface.positional.at(argument_index);
        }

        /**
         * Describes why one overload candidate did not accept the call arguments.
         */
        auto co_describe_invalid_call_candidate(type_symbol const& func, temploid_ensig const& ensig, instatype const& params, allowed_adaptations adaptations) -> co_type< std::string >
        {
            std::string note = "  candidate: " + to_string(func) + " " + to_string(ensig.interface);
            type_symbol type_of_this = void_type{};
            if (typeis< submember >(func))
            {
                type_of_this = as< submember >(func).of;
            }

            std::optional< instatype > const accepted = co_await rpnx::querygraph::request< function_ensig_init_with_query >(ensig_initialization{
                .ensig = ensig,
                .params = params,
                .adaptations = adaptations,
                .type_of_this = type_of_this,
            });
            if (accepted.has_value())
            {
                co_return note + "\n    note: viable before overload filtering";
            }

            std::optional< std::size_t > const pack_index = this->call_candidate_positional_pack_index(ensig);
            std::size_t const fixed_positional_count = pack_index.value_or(ensig.interface.positional.size());
            if (!pack_index.has_value() && params.positional.size() > ensig.interface.positional.size())
            {
                co_return note + "\n    note: too many positional arguments; expected " + std::to_string(ensig.interface.positional.size()) + ", got " + std::to_string(params.positional.size());
            }
            if (pack_index.has_value() && params.positional.size() < fixed_positional_count)
            {
                co_return note + "\n    note: too few positional arguments before variadic pack; expected at least " + std::to_string(fixed_positional_count) + ", got " + std::to_string(params.positional.size());
            }

            for (std::pair< std::string const, parameter_instantiation > const& actual : params.named)
            {
                std::map< std::string, argif >::const_iterator const formal_iter = ensig.interface.named.find(actual.first);
                if (formal_iter == ensig.interface.named.end())
                {
                    co_return note + "\n    note: unknown named argument @" + actual.first;
                }

                bool const actual_is_value = actual.second.template type_is< parameter_value_instantiation >();
                if (formal_iter->second.requires_static_value != actual_is_value)
                {
                    co_return note + "\n    note: named argument @" + actual.first + (formal_iter->second.requires_static_value ? " requires a static value" : " does not accept a static value");
                }

                type_symbol const& actual_type = parameter_instantiation_type(actual.second);
                std::optional< type_symbol > initialized_type;
                if (adaptations == allowed_adaptations::none)
                {
                    if (match_template(formal_iter->second.type, actual_type).has_value())
                    {
                        initialized_type = actual_type;
                    }
                }
                else
                {
                    initialized_type = co_await rpnx::querygraph::request< ensig_argument_initialize_query >(argument_init_input{
                        .from = actual_type,
                        .to = formal_iter->second.type,
                        .adaptations = adaptations,
                    });
                }

                if (!initialized_type.has_value())
                {
                    co_return note + "\n    note: named argument @" + actual.first + " cannot initialize " + to_string(formal_iter->second.type) + " from " + to_string(actual_type);
                }
                if (!match_template(formal_iter->second.type, *initialized_type).has_value())
                {
                    co_return note + "\n    note: named argument @" + actual.first + " initializes to " + to_string(*initialized_type) + " but does not satisfy template pattern " + to_string(formal_iter->second.type);
                }
            }

            for (std::pair< std::string const, argif > const& formal : ensig.interface.named)
            {
                if (!params.named.contains(formal.first) && !formal.second.is_defaulted)
                {
                    co_return note + "\n    note: missing required named argument @" + formal.first;
                }
            }

            for (std::size_t i = 0; i < params.positional.size(); i++)
            {
                if (!pack_index.has_value() && i >= ensig.interface.positional.size())
                {
                    co_return note + "\n    note: unexpected positional argument " + std::to_string(i);
                }

                argif const& formal = this->call_candidate_positional_formal_for(ensig, i);
                parameter_instantiation const& actual = params.positional.at(i);
                bool const actual_is_value = actual.template type_is< parameter_value_instantiation >();
                if (formal.requires_static_value != actual_is_value)
                {
                    co_return note + "\n    note: positional argument " + std::to_string(i) + (formal.requires_static_value ? " requires a static value" : " does not accept a static value");
                }

                type_symbol const& actual_type = parameter_instantiation_type(actual);
                std::optional< type_symbol > initialized_type;
                if (adaptations == allowed_adaptations::none)
                {
                    if (match_template(formal.type, actual_type).has_value())
                    {
                        initialized_type = actual_type;
                    }
                }
                else
                {
                    initialized_type = co_await rpnx::querygraph::request< ensig_argument_initialize_query >(argument_init_input{
                        .from = actual_type,
                        .to = formal.type,
                        .adaptations = adaptations,
                    });
                }

                if (!initialized_type.has_value())
                {
                    co_return note + "\n    note: positional argument " + std::to_string(i) + " cannot initialize " + to_string(formal.type) + " from " + to_string(actual_type);
                }
                if (!match_template(formal.type, *initialized_type).has_value())
                {
                    co_return note + "\n    note: positional argument " + std::to_string(i) + " initializes to " + to_string(*initialized_type) + " but does not satisfy template pattern " + to_string(formal.type);
                }
            }

            if (!pack_index.has_value())
            {
                for (std::size_t i = params.positional.size(); i < ensig.interface.positional.size(); i++)
                {
                    if (!ensig.interface.positional.at(i).is_defaulted)
                    {
                        co_return note + "\n    note: missing required positional argument " + std::to_string(i);
                    }
                }
            }

            co_return note + "\n    note: not viable after template deduction or default argument checks";
        }

        /**
         * Builds an invalid-call diagnostic with overload candidate notes.
         */
        auto co_invalid_call_message(type_symbol const& func, invotype const& calltype, instatype const& params, std::set< temploid_ensig > const& overloads, allowed_adaptations adaptations)
            -> co_type< std::string >
        {
            std::string message = "Cannot call " + to_string(func) + " with " + quxlang::to_string(calltype);
            if (overloads.empty())
            {
                co_return message + "\ncandidates: none";
            }

            message += "\ncandidates:";
            for (temploid_ensig const& overload : overloads)
            {
                message += "\n";
                message += co_await this->co_describe_invalid_call_candidate(func, overload, params, adaptations);
            }
            co_return message;
        }

        auto co_gen_construct_with_target_type(block_index& bidx, value_index source, type_symbol target_type, allowed_adaptations adaptations) -> co_type< value_index >
        {
            auto target_index = create_local_value(target_type);
            auto constructor_functum = submember{target_type, "CONSTRUCTOR"};
            auto constructor_adaptations = nested_constructor_adaptations(adaptations);

            codegen_invocation_args ctor_args = {.named = {{"THIS", target_index}, {"OTHER", source}}};
            co_await co_gen_call_functum(bidx, constructor_functum, ctor_args, constructor_adaptations);

            co_return target_index;
        }

        auto co_gen_call_functum(block_index& bidx, type_symbol func, codegen_invocation_args args, allowed_adaptations adaptations = allowed_adaptations::destination_rebinding) -> co_type< value_index >
        {
            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                co_yield rpnx::querygraph::debug_message("co_gen_call_functum({}){}", quxlang::to_string(func), quxlang::to_string(args));
            }

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
                if (name == "THIS" && typeis< attached_type_reference >(arg_type))
                {
                    arg_type = as< attached_type_reference >(arg_type).carrying_type;
                }
                bool is_alive = value_alive(bidx, arg);

                if constexpr (QUXLANG_IN_DEBUG)
                {
                    std::string name_copy = name;
                    std::string arg_type_str = to_string(arg_type);

                    // co_yield rpnx::querygraph::debug_message(" arg name={} index={} is_alive={} current_type={}", name, arg, is_alive, to_string(arg_type));
                }
                if (!is_alive)
                {
                    assert(typeis< nvalue_slot >(arg_type));
                }

                calltype.named[name] = arg_type;
            }

            instatype call_parameters = instatype_from_invotype(calltype);
            initialization_reference functanoid_unnormalized{.initializee = func, .parameters = call_parameters, .adaptations = adaptations};

            // co_yield rpnx::querygraph::debug_message("co_gen_call_functum initialization params: ({})", quxlang::to_string(functanoid_unnormalized));
            //  Get call type

            auto kind = (co_await rpnx::querygraph::request< symbol_type_query >(func));
            if (kind != symbol_kind::functum)
            {

                if (kind == symbol_kind::noexist)
                {
                    auto func_str = to_string(func);
                    throw semantic_compilation_error(func_str + " does not exist");
                }
                else if (kind == symbol_kind::local_variable)
                {
                    auto func_str = to_string(func);
                    throw semantic_compilation_error("Error: cannot call local variable " + func_str + " as a functum");
                }
                else if (kind == symbol_kind::class_)
                {
                    auto func_str = to_string(func);
                    throw semantic_compilation_error("Error: cannot call class type " + func_str + " as a functum");
                }
                else
                {
                    auto func_str = to_string(func);
                    throw semantic_compilation_error("Error: symbol " + func_str + " is not a functum");
                }
            }
            auto instanciation = co_await rpnx::querygraph::request< instanciation_query >(functanoid_unnormalized);

            auto functum_overloeads = co_await rpnx::querygraph::request< functum_overloads_query >(func);

            for (auto const& overload : functum_overloeads)
            {
                if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
                {
                    co_yield rpnx::querygraph::debug_message(" - Candidate: {}", to_string(overload.interface));
                }
            }

            if (!instanciation)
            {
                throw semantic_compilation_error(co_await this->co_invalid_call_message(func, calltype, call_parameters, functum_overloeads, adaptations));
            }

            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                co_yield rpnx::querygraph::debug_message("co_gen_call_functum selected instanciation: {}", quxlang::to_string(*instanciation));
            }

            co_return co_await this->co_gen_call_functanoid(bidx, instanciation.value(), args, adaptations);
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

        /// Returns true when a runtime/codegen type still contains THISTYPE outside allowed symbol-identity positions.
        static auto has_forbidden_codegen_thistype(type_symbol const& type, bool in_symbol_identity = false) -> bool
        {
            if (typeis< thistype >(type))
            {
                return !in_symbol_identity;
            }
            if (typeis< ptrref_type >(type))
            {
                return has_forbidden_codegen_thistype(as< ptrref_type >(type).target, in_symbol_identity);
            }
            if (typeis< nvalue_slot >(type))
            {
                return has_forbidden_codegen_thistype(as< nvalue_slot >(type).target, in_symbol_identity);
            }
            if (typeis< dvalue_slot >(type))
            {
                return has_forbidden_codegen_thistype(as< dvalue_slot >(type).target, in_symbol_identity);
            }
            if (typeis< attached_type_reference >(type))
            {
                attached_type_reference const& attached = as< attached_type_reference >(type);
                return has_forbidden_codegen_thistype(attached.carrying_type, in_symbol_identity) || has_forbidden_codegen_thistype(attached.attached_symbol, in_symbol_identity);
            }
            if (typeis< array_type >(type))
            {
                return has_forbidden_codegen_thistype(as< array_type >(type).element_type, in_symbol_identity);
            }
            if (typeis< array_initializer_type >(type))
            {
                return has_forbidden_codegen_thistype(as< array_initializer_type >(type).element_type, in_symbol_identity);
            }
            if (typeis< procedure_type >(type))
            {
                procedure_type const& proc = as< procedure_type >(type);
                for (auto const& [name, param_type] : proc.signature.params.named)
                {
                    (void)name;
                    if (has_forbidden_codegen_thistype(param_type, in_symbol_identity))
                    {
                        return true;
                    }
                }
                for (type_symbol const& param_type : proc.signature.params.positional)
                {
                    if (has_forbidden_codegen_thistype(param_type, in_symbol_identity))
                    {
                        return true;
                    }
                }
                if (proc.signature.return_type.has_value() && has_forbidden_codegen_thistype(*proc.signature.return_type, in_symbol_identity))
                {
                    return true;
                }
                return false;
            }
            if (typeis< storage >(type))
            {
                for (type_symbol const& storable_type : as< storage >(type).storable_types)
                {
                    if (has_forbidden_codegen_thistype(storable_type, in_symbol_identity))
                    {
                        return true;
                    }
                }
                return false;
            }
            if (typeis< initialization_reference >(type))
            {
                initialization_reference const& init = as< initialization_reference >(type);
                if (has_forbidden_codegen_thistype(init.initializee, in_symbol_identity))
                {
                    return true;
                }
                if (init.context.has_value() && has_forbidden_codegen_thistype(*init.context, in_symbol_identity))
                {
                    return true;
                }
                for (parameter_instantiation const& param : init.parameters.positional)
                {
                    if (has_forbidden_codegen_thistype(parameter_instantiation_type(param), in_symbol_identity))
                    {
                        return true;
                    }
                }
                for (auto const& [name, param] : init.parameters.named)
                {
                    (void)name;
                    if (has_forbidden_codegen_thistype(parameter_instantiation_type(param), in_symbol_identity))
                    {
                        return true;
                    }
                }
                return false;
            }
            if (typeis< subsymbol >(type))
            {
                return has_forbidden_codegen_thistype(as< subsymbol >(type).of, true);
            }
            if (typeis< submember >(type))
            {
                return has_forbidden_codegen_thistype(as< submember >(type).of, true);
            }
            if (typeis< temploid_reference >(type))
            {
                return has_forbidden_codegen_thistype(as< temploid_reference >(type).templexoid, true);
            }
            if (typeis< instanciation_reference >(type))
            {
                instanciation_reference const& inst = as< instanciation_reference >(type);
                if (has_forbidden_codegen_thistype(inst.temploid.templexoid, true))
                {
                    return true;
                }
                for (parameter_instantiation const& param : inst.params.positional)
                {
                    if (has_forbidden_codegen_thistype(parameter_instantiation_type(param), true))
                    {
                        return true;
                    }
                }
                for (auto const& [name, param] : inst.params.named)
                {
                    (void)name;
                    if (has_forbidden_codegen_thistype(parameter_instantiation_type(param), true))
                    {
                        return true;
                    }
                }
                return false;
            }
            if (typeis< static_local_ref >(type))
            {
                return has_forbidden_codegen_thistype(as< static_local_ref >(type).functanoid, true);
            }
            if (typeis< static_snapshot_ref >(type))
            {
                return has_forbidden_codegen_thistype(as< static_snapshot_ref >(type).functanoid, true);
            }
            if (typeis< decltype_type_ref >(type))
            {
                return has_forbidden_codegen_thistype(as< decltype_type_ref >(type).symbol, in_symbol_identity);
            }
            return false;
        }

        /// Throws a compiler bug when THISTYPE reaches a concrete runtime/codegen surface.
        static auto validate_codegen_type(type_symbol const& type, std::string_view context) -> void
        {
            if (has_forbidden_codegen_thistype(type))
            {
                throw compiler_bug(std::string(context) + " cannot contain THISTYPE outside of member/lambda symbol identities: " + to_string(type));
            }
        }

        auto create_local_value(type_symbol type) -> value_index
        {
            // Locals cannot be nvalue or dvalue slots, that is only the case for parameters.
            assert(!type.template type_is< nvalue_slot >() && !type.template type_is< dvalue_slot >());
            validate_codegen_type(type, "Codegen local type");
            codegen_local storage;
            this->state.locals.push_back(vmir2::local_type{.type = type});
            storage.local_index = local_index(this->state.locals.size() - 1);
            this->state.genvalues.push_back(storage);
            return value_index(this->state.genvalues.size() - 1);
        }

        auto co_gen_get_procedure_ptr(block_index& bidx, type_symbol routine, std::string calling_convention) -> co_type< value_index >
        {
            procedure_type proc_type;
            proc_type.calling_convention = std::move(calling_convention);

            if (typeis< instanciation_reference >(routine))
            {
                auto const& functanoid = as< instanciation_reference >(routine);
                auto concrete_params = co_await rpnx::querygraph::request< instanciation_concrete_params_query >(functanoid);
                proc_type.signature.params = invotype_from_instatype(concrete_params);
                proc_type.signature.return_type = co_await rpnx::querygraph::request< functanoid_return_type_query >(functanoid);
            }
            else if (typeis< temploid_reference >(routine))
            {
                auto const& selected_function = as< temploid_reference >(routine);
                auto formal_ensig = co_await rpnx::querygraph::request< temploid_formal_ensig_query >(selected_function);
                if (!formal_ensig.has_value())
                {
                    throw semantic_compilation_error("Procedure pointer target function overload not found");
                }

                for (auto const& arg : formal_ensig->interface.positional)
                {
                    if (arg.is_pack)
                    {
                        throw semantic_compilation_error("Cannot form a procedure pointer to an uninstantiated variadic function");
                    }
                    proc_type.signature.params.positional.push_back(arg.type);
                }
                for (auto const& [name, arg] : formal_ensig->interface.named)
                {
                    if (arg.is_pack)
                    {
                        throw semantic_compilation_error("Cannot form a procedure pointer to an uninstantiated variadic function");
                    }
                    proc_type.signature.params.named[name] = arg.type;
                }

                auto decl = co_await rpnx::querygraph::request< function_declaration_query >(selected_function);
                if (!decl.has_value())
                {
                    throw semantic_compilation_error("Procedure pointer target function declaration not found");
                }

                auto ret_type = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                    .context = selected_function,
                    .type = decl->definition.return_type.value_or(type_symbol(void_type{})),
                });
                if (!ret_type.has_value())
                {
                    throw semantic_compilation_error("Procedure pointer target return type could not be resolved");
                }
                proc_type.signature.return_type = ret_type.value();
            }
            else
            {
                throw semantic_compilation_error("Procedure pointers currently require a concrete function selection");
            }

            auto pointer_value = create_local_value(ptrref_type{
                .target = proc_type,
                .ptr_class = pointer_class::instance,
                .qual = qualifier::constant,
            });

            vmir2::get_procedure_ptr get_proc_ptr;
            get_proc_ptr.routine = routine;
            get_proc_ptr.calling_convention = proc_type.calling_convention;
            get_proc_ptr.pointer_index = get_local_index(pointer_value);
            this->emit(bidx, get_proc_ptr);

            co_return pointer_value;
        }

        auto resolve_functum_instanciation(block_index& bidx, type_symbol func, invotype calltype, allowed_adaptations adaptations) -> co_type< instanciation_reference >
        {
            instatype call_parameters = instatype_from_invotype(calltype);
            initialization_reference functanoid_unnormalized{.initializee = func, .parameters = call_parameters, .adaptations = adaptations};

            auto kind = (co_await rpnx::querygraph::request< symbol_type_query >(func));
            if (kind != symbol_kind::functum)
            {
                throw semantic_compilation_error("Expected functum symbol " + to_string(func));
            }

            auto instanciation = co_await rpnx::querygraph::request< instanciation_query >(functanoid_unnormalized);
            if (!instanciation)
            {
                std::set< temploid_ensig > const functum_overloads = co_await rpnx::querygraph::request< functum_overloads_query >(func);
                throw semantic_compilation_error(co_await this->co_invalid_call_message(func, calltype, call_parameters, functum_overloads, adaptations));
            }

            co_return *instanciation;
        }

        auto adapt_args_for_instanciation(block_index& bidx, instanciation_reference what, codegen_invocation_args expression_args, std::set< std::string > skip_named = {}) -> co_type< codegen_invocation_args >
        {
            codegen_invocation_args invocation_args;
            auto concrete_params = co_await rpnx::querygraph::request< instanciation_concrete_params_query >(what);

            auto create_arg_value = [&](value_index arg_expr_index, type_symbol arg_target_type) -> co_type< value_index >
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

                co_return co_await co_gen_argument_adaptation(bidx, arg_expr_index, arg_target_type, allowed_adaptations::destination_rebinding);
            };

            for (auto const& [name, arg_accepted_type] : concrete_params.named)
            {
                if (skip_named.contains(name))
                {
                    continue;
                }

                auto arg_expr_index = expression_args.named.at(name);
                invocation_args.named[name] = co_await create_arg_value(arg_expr_index, parameter_instantiation_type(arg_accepted_type));
            }

            std::size_t positional_write = 0;
            for (std::size_t i = 0; i < concrete_params.positional.size(); i++)
            {
                auto arg_accepted_type = parameter_instantiation_type(concrete_params.positional.at(i));
                auto arg_expr_index = expression_args.positional.at(positional_write++);
                invocation_args.positional.push_back(co_await create_arg_value(arg_expr_index, arg_accepted_type));
            }

            co_return invocation_args;
        }

        auto co_expect_storage_reference(block_index bidx, value_index storage_ref, bool require_mut, std::optional< type_symbol > projected_type = std::nullopt) -> co_type< ptrref_type >
        {
            auto storage_ref_type = this->current_type(bidx, storage_ref);
            if (!is_ref(storage_ref_type))
            {
                throw semantic_compilation_error("Expected a storage reference");
            }

            auto ref_type = as< ptrref_type >(storage_ref_type);
            if (require_mut && ref_type.qual != qualifier::mut)
            {
                throw semantic_compilation_error("Expected MUT& STORAGE for storage mutation");
            }

            auto storage_type = remove_ref(storage_ref_type);
            if (!typeis< storage >(storage_type) && !typeis< aligned_storage >(storage_type))
            {
                throw semantic_compilation_error("Expected a storage-typed reference");
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
                    throw semantic_compilation_error("Projected type is not permitted by storage type");
                }
            }
            else if (projected_type.has_value() && typeis< aligned_storage >(storage_type))
            {
                auto projected_placement = co_await rpnx::querygraph::request< type_placement_info_query >(*projected_type);
                auto storage_placement = co_await rpnx::querygraph::request< type_placement_info_query >(storage_type);
                if (projected_placement.size > storage_placement.size || projected_placement.alignment > storage_placement.alignment)
                {
                    throw semantic_compilation_error("Projected type does not fit in aligned storage");
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

        /// Returns the innermost visible static-local symbol for a source name.
        auto find_visible_static_binding(std::string const& name) const -> std::optional< static_local_ref >
        {
            for (auto scope_it = this->state.static_scopes.rbegin(); scope_it != this->state.static_scopes.rend(); ++scope_it)
            {
                auto it = scope_it->bindings.find(name);
                if (it != scope_it->bindings.end())
                {
                    return it->second;
                }
            }
            return std::nullopt;
        }

        /// Returns true when a stable static-local symbol is tracked by this generator.
        auto has_static_binding(static_local_ref const& symbol) const -> bool
        {
            return this->state.statics.contains(symbol);
        }

        /// Emits GET_ANTESTATAL_REF and returns a local reference with the requested mutability.
        auto create_antestatal_reference(block_index& bidx, type_symbol symbol, type_symbol type, bool is_mutable) -> value_index
        {
            auto ref_type = is_mutable ? make_mref(type) : make_cref(type);
            auto ref = this->create_local_value(ref_type);
            this->emit(bidx, vmir2::get_antestatal_ref{.symbol = std::move(symbol), .target_ref = get_local_index(ref)});
            return ref;
        }

        /// Rewrites function-local static references inside a snapshot pointer graph.
        auto rewrite_antestatal_access_for_snapshot(antestatal_access access, std::map< static_local_ref, static_snapshot_ref >& remapped, bool allow_mutable_static_targets) -> antestatal_access
        {
            if (typeis< antestatal_access_global >(access))
            {
                auto& global = as< antestatal_access_global >(access);
                if (typeis< static_local_ref >(global.symbol))
                {
                    auto const& local_symbol = as< static_local_ref >(global.symbol);
                    if (has_static_binding(local_symbol))
                    {
                        global.symbol = type_symbol(create_ordinary_snapshot_for_binding(local_symbol, remapped, allow_mutable_static_targets));
                    }
                }
                return access;
            }
            if (typeis< antestatal_access_field >(access))
            {
                auto& field = as< antestatal_access_field >(access);
                field.object = rewrite_antestatal_access_for_snapshot(std::move(field.object), remapped, allow_mutable_static_targets);
                return access;
            }
            if (typeis< antestatal_access_array_element >(access))
            {
                auto& element = as< antestatal_access_array_element >(access);
                element.array = rewrite_antestatal_access_for_snapshot(std::move(element.array), remapped, allow_mutable_static_targets);
                return access;
            }
            return access;
        }

        /// Copies an antestatal value while remapping function-local static pointer targets.
        auto rewrite_antestatal_value_for_snapshot(antestatal_value value, std::map< static_local_ref, static_snapshot_ref >& remapped, bool allow_mutable_static_targets) -> antestatal_value
        {
            if (typeis< antestatal_ptrref >(value))
            {
                auto& ptr = as< antestatal_ptrref >(value);
                ptr.target = rewrite_antestatal_access_for_snapshot(std::move(ptr.target), remapped, allow_mutable_static_targets);
                return value;
            }
            if (typeis< antestatal_array >(value))
            {
                auto& arr = as< antestatal_array >(value);
                for (auto& element : arr.elements)
                {
                    element = rewrite_antestatal_value_for_snapshot(std::move(element), remapped, allow_mutable_static_targets);
                }
                return value;
            }
            if (typeis< antestatal_struct >(value))
            {
                auto& st = as< antestatal_struct >(value);
                for (auto& [_, field] : st.fields)
                {
                    field = rewrite_antestatal_value_for_snapshot(std::move(field), remapped, allow_mutable_static_targets);
                }
                return value;
            }
            return value;
        }

        /// Creates immutable localdata for a runtime read of a function-local static binding.
        auto create_ordinary_snapshot_for_binding(static_local_ref const& symbol, std::map< static_local_ref, static_snapshot_ref >& remapped, bool allow_mutable_static_targets) -> static_snapshot_ref
        {
            auto const& binding = this->state.statics.at(symbol);
            if (binding.mutation_result_id.has_value() && !allow_mutable_static_targets)
            {
                throw semantic_compilation_error("cannot use mutable function-local static outside constexpr context without SNAPSHOT: " + symbol.name);
            }
            if (auto it = remapped.find(symbol); it != remapped.end())
            {
                return it->second;
            }

            static_snapshot_ref snapshot_symbol{.functanoid = this->ctx, .name = symbol.name, .generation = symbol.generation, .snapshot_id = this->state.next_static_snapshot_id++};
            remapped[symbol] = snapshot_symbol;
            auto snapshot_value = rewrite_antestatal_value_for_snapshot(constexpr_value_as_antestatal(binding.value), remapped, allow_mutable_static_targets);
            this->state.static_snapshots[snapshot_symbol] = vmir2::localdata_entry{
                .type = binding.type,
                .value = std::move(snapshot_value),
                .is_mutable = false,
            };
            return snapshot_symbol;
        }

        auto create_binding(value_index bindval, type_symbol bind_type)
        {
            assert(!type_is_contextual(bind_type));
            validate_codegen_type(bind_type, "Codegen binding type");
            codegen_binding binding;
            binding.attached_symbol = bind_type;
            binding.bound_value = bindval;
            this->state.genvalues.push_back(binding);
            return value_index(this->state.genvalues.size() - 1);
        }

        static auto runtime_type_for_attached_parameter(type_symbol const& param_type) -> std::optional< type_symbol >
        {
            if (!typeis< attached_type_reference >(param_type))
            {
                return param_type;
            }

            attached_type_reference const& attached = as< attached_type_reference >(param_type);
            if (typeis< void_type >(attached.carrying_type))
            {
                return std::nullopt;
            }
            return attached.carrying_type;
        }

        static auto storage_type_for_attached_field(type_symbol const& field_type) -> std::optional< type_symbol >
        {
            if (!typeis< attached_type_reference >(field_type))
            {
                return field_type;
            }

            attached_type_reference const& attached = as< attached_type_reference >(field_type);
            if (typeis< void_type >(attached.carrying_type))
            {
                return std::nullopt;
            }
            return attached.carrying_type;
        }

        auto attached_binding_carrier_value(block_index& current_block, value_index binding_value, type_symbol const& expected_type) -> value_index
        {
            type_symbol binding_type = this->current_type(current_block, binding_value);
            if (binding_type != expected_type)
            {
                throw semantic_compilation_error("Expected attached binding " + to_string(expected_type) + ", got " + to_string(binding_type));
            }
            if (!this->state.genvalues.at(binding_value).template type_is< codegen_binding >())
            {
                throw semantic_compilation_error("Expected attached binding value");
            }

            codegen_binding const& binding = this->state.genvalues.at(binding_value).template get_as< codegen_binding >();
            if (binding.bound_value == value_index(0))
            {
                throw semantic_compilation_error("Expected bound attached binding value");
            }
            return binding.bound_value;
        }

        static auto parameter_local_type(type_symbol param_type) -> type_symbol
        {
            type_symbol local_type = std::move(param_type);
            if (typeis< nvalue_slot >(local_type))
            {
                local_type = type_symbol(as< nvalue_slot >(local_type).target);
            }
            else if (typeis< dvalue_slot >(local_type))
            {
                local_type = as< dvalue_slot >(local_type).target;
            }
            return local_type;
        }

        auto copy_ref_value(block_index& current_block, value_index val) -> value_index
        {
            type_symbol val_type = this->current_type(current_block, val);
            if (!val_type.type_is< ptrref_type >())
            {
                throw compiler_bug("Expected a reference type");
            }

            ptrref_type const& vptr = val_type.get_as< ptrref_type >();
            if (vptr.ptr_class != pointer_class::ref)
            {
                throw compiler_bug("Expected a reference type");
            }

            value_index copy_idx = this->create_local_value(vptr);
            this->emit(current_block, vmir2::copy_reference{.from_index = get_local_index(val), .to_index = get_local_index(copy_idx)});
            return copy_idx;
        }

        auto copy_attached_binding_value(block_index& current_block, value_index binding_value, std::optional< type_symbol > expected_type) -> value_index
        {
            type_symbol binding_type = this->current_type(current_block, binding_value);
            if (expected_type.has_value() && binding_type != *expected_type)
            {
                throw semantic_compilation_error("Cannot copy attached binding " + to_string(binding_type) + " into " + to_string(*expected_type));
            }

            if (!typeis< attached_type_reference >(binding_type))
            {
                throw semantic_compilation_error("Expected attached binding type, got " + to_string(binding_type));
            }
            if (!this->state.genvalues.at(binding_value).template type_is< codegen_binding >())
            {
                throw semantic_compilation_error("Expected attached binding value");
            }

            codegen_binding const& binding = this->state.genvalues.at(binding_value).template get_as< codegen_binding >();
            if (binding.bound_value == value_index(0))
            {
                return this->create_binding(value_index(0), binding.attached_symbol);
            }

            type_symbol carrier_type = this->current_type(current_block, binding.bound_value);
            value_index carrier_copy = binding.bound_value;
            if (is_ref(carrier_type))
            {
                carrier_copy = this->copy_ref_value(current_block, binding.bound_value);
            }

            return this->create_binding(carrier_copy, binding.attached_symbol);
        }

        auto co_copy_attached_binding(block_index& current_block, value_index binding_value, type_symbol expected_type) -> co_type< value_index >
        {
            co_return this->copy_attached_binding_value(current_block, binding_value, std::move(expected_type));
        }

        /// This is a very low level function, generally DO NOT USE IT
        /// 99% of instruction output should be through intrinsic functions
        void emit(block_index& bidx, vmir2::vm_instruction val)
        {
            codegen_block& block = this->state.blocks.at(bidx);
            // val.from = get_local_index(val.from);
            // ... (do this for all relevant fields)
            this->apply_current_source_location(val);
            try
            {
                vmir2::codegen_state_engine(this->state.blocks.at(bidx).current_state, this->state.locals, this->state.params).apply(val);
            }
            catch (invalid_instruction_transition_error const& error)
            {
                vmir2::functanoid_routine3 partial_routine;
                partial_routine.local_types = this->state.locals;
                partial_routine.parameters = this->state.params;
                auto instruction = vmir2::assembler(partial_routine).to_string(val);
                throw invalid_instruction_transition_error(std::string(error.what()) + " while emitting " + instruction);
            }

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

        auto cast_ptrref(block_index bidx, value_index index, type_symbol const& ty)
        {
            auto temp = create_local_value(ty);
            vmir2::cast_ptrref ref;
            ref.source_index = get_local_index(index);
            ref.target_index = get_local_index(temp);
            this->emit(bidx, ref);
            return temp;
        }

        /// Converts a local value lookup result into the reference form expected by expression reads.
        auto materialize_lookup_reference(block_index idx, value_index lookup) -> value_index
        {
            auto lookup_type = this->current_type(idx, lookup);

            if (typeis< attached_type_reference >(lookup_type))
            {
                return this->copy_attached_binding_value(idx, lookup, lookup_type);
            }

            if (!is_ref(lookup_type))
            {
                lookup = create_reference(idx, lookup, make_mref(lookup_type));
            }
            else
            {
                lookup = copy_refernece_internal(idx, lookup);

                lookup_type = this->current_type(idx, lookup);

                auto const& lookup_type_ref = as< ptrref_type >(lookup_type);

                if (lookup_type_ref.qual == qualifier::write)
                {
                    lookup = cast_ptrref(idx, lookup, make_mref(remove_ref(lookup_type)));
                }
            }

            assert(!type_is_contextual(this->current_type(idx, lookup)));
            return lookup;
        }

        auto get_value_index(local_index local) -> value_index
        {
            for (std::size_t i = 1; i < this->state.genvalues.size(); i++)
            {
                auto const& value = this->state.genvalues.at(i);
                if (value.template type_is< codegen_local >() && value.template get_as< codegen_local >().local_index == local)
                {
                    return value_index(i);
                }
            }
            throw compiler_bug("No codegen value found for local index");
        }

        auto declared_type_of_local_value(value_index lookup) -> type_symbol
        {
            while (this->state.genvalues.at(lookup).template type_is< codegen_binding >())
            {
                lookup = this->state.genvalues.at(lookup).template get_as< codegen_binding >().bound_value;
            }

            local_index local = get_local_index(lookup);
            for (auto const& parameter : this->state.params.positional)
            {
                if (parameter.local_index == local)
                {
                    return parameter.type;
                }
            }
            for (auto const& [_, parameter] : this->state.params.named)
            {
                if (parameter.local_index == local)
                {
                    return parameter.type;
                }
            }
            return this->state.locals.at(local).type;
        }

        auto co_lookup_declared_symbol_type(block_index idx, type_symbol symbol) -> co_type< type_symbol >
        {
            if (symbol.template type_is< freebound_identifier >())
            {
                auto const& name = symbol.template get_as< freebound_identifier >().name;
                auto local = this->local_value_direct_lookup(idx, name);
                if (local.has_value())
                {
                    co_return declared_type_of_local_value(*local);
                }
            }

            auto canonical_symbol = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = ctx, .type = std::move(symbol)});
            if (!canonical_symbol.has_value())
            {
                throw semantic_compilation_error("DECLTYPE target symbol could not be resolved");
            }

            auto kind = co_await rpnx::querygraph::request< symbol_type_query >(*canonical_symbol);
            if (kind != symbol_kind::global_variable)
            {
                throw semantic_compilation_error("DECLTYPE requires a value symbol");
            }

            auto sym = co_await rpnx::querygraph::request< symboid_query >(*canonical_symbol);
            if (!sym.template type_is< ast2_variable_declaration >())
            {
                throw semantic_compilation_error("DECLTYPE target variable is not declared as a variable");
            }

            auto declared_type = sym.template get_as< ast2_variable_declaration >().type;
            auto resolved = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = *canonical_symbol, .type = declared_type});
            if (!resolved.has_value())
            {
                throw semantic_compilation_error("DECLTYPE target type could not be resolved");
            }
            co_return *resolved;
        }

        auto co_resolve_type_symbol(block_index& idx, type_symbol type) -> co_type< type_symbol >
        {
            if (type.template type_is< decltype_type_ref >())
            {
                co_return co_await this->co_lookup_declared_symbol_type(idx, type.template get_as< decltype_type_ref >().symbol);
            }
            if (type.template type_is< typeof_type_ref >())
            {
                codegen_state saved_state = this->state;
                block_index const saved_idx = idx;
                try
                {
                    auto value = co_await this->co_generate_expr(idx, type.template get_as< typeof_type_ref >().expr);
                    type_symbol result = this->current_type(idx, value);
                    this->state = std::move(saved_state);
                    idx = saved_idx;
                    co_return result;
                }
                catch (...)
                {
                    this->state = std::move(saved_state);
                    idx = saved_idx;
                    throw;
                }
            }
            if (type.template type_is< ptrref_type >())
            {
                auto ref = type.template get_as< ptrref_type >();
                ref.target = co_await this->co_resolve_type_symbol(idx, std::move(ref.target));
                auto resolved = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = ctx, .type = std::move(ref)});
                if (!resolved.has_value())
                {
                    throw semantic_compilation_error("Type could not be resolved");
                }
                co_return *resolved;
            }
            if (type.template type_is< array_type >())
            {
                auto array = type.template get_as< array_type >();
                array.element_type = co_await this->co_resolve_type_symbol(idx, std::move(array.element_type));
                auto resolved = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = ctx, .type = std::move(array)});
                if (!resolved.has_value())
                {
                    throw semantic_compilation_error("Array type could not be resolved");
                }
                co_return *resolved;
            }
            if (type.template type_is< storage >())
            {
                storage result;
                for (auto item : type.template get_as< storage >().storable_types)
                {
                    result.storable_types.insert(co_await this->co_resolve_type_symbol(idx, std::move(item)));
                }
                co_return result;
            }

            auto resolved = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = ctx, .type = std::move(type)});
            if (!resolved.has_value())
            {
                throw semantic_compilation_error("Type could not be resolved");
            }
            co_return *resolved;
        }

        // Look up a type/class symbol in the current codegen context.
        // Uses co_lookup_symbol to respect local tempar type definitions.
        // Errors if the symbol resolves to a value binding or does not refer to a class.
        auto co_lookup_typeclass(block_index idx, type_symbol sym) -> co_type< type_symbol >
        {

            auto looked = co_await co_lookup_symbol(idx, sym);
            if (!looked.has_value())
            {
                throw semantic_compilation_error("Type not found: " + to_string(sym));
            }

            auto val = looked.value();
            auto vtype = this->current_type(idx, val);

            if (!typeis< attached_type_reference >(vtype))
            {
                throw semantic_compilation_error("Lookup did not yield a type symbol: " + to_string(vtype));
            }

            auto const& att = as< attached_type_reference >(vtype);
            // If carrying_type is not void, it's a member bound to a value.
            if (!typeis< void_type >(att.carrying_type))
            {
                throw semantic_compilation_error("Type symbol bound to a value: " + to_string(vtype));
            }

            auto kind = co_await rpnx::querygraph::request< symbol_type_query >(att.attached_symbol);
            if (kind != symbol_kind::class_)
            {
                throw semantic_compilation_error("Symbol is not a class: " + to_string(att.attached_symbol));
            }

            co_return att.attached_symbol;
        }

        static auto option_bool_value(std::string const& value) -> std::optional< bool >
        {
            if (value == "TRUE" || value == "true" || value == "1" || value == "on" || value == "enable" || value == "enabled")
            {
                return true;
            }
            if (value == "FALSE" || value == "false" || value == "0" || value == "off" || value == "disabled")
            {
                return false;
            }
            return std::nullopt;
        }

        auto create_configured_option_value(block_index idx, ast2_option const& option, std::string const& option_value, type_symbol const& option_symbol) -> value_index
        {
            if (option.kind == option_kind::number)
            {
                return this->create_numeric_literal(option_value);
            }
            if (option.kind == option_kind::string)
            {
                return this->create_string_literal(option_value);
            }
            if (option.kind == option_kind::boolean)
            {
                auto bool_value = option_bool_value(option_value);
                if (!bool_value.has_value())
                {
                    throw semantic_compilation_error("Invalid BOOL option value for " + to_string(option_symbol) + ": " + option_value);
                }
                return this->create_bool_value(idx, *bool_value);
            }

            throw compiler_bug("Unhandled option kind");
        }

        auto create_default_option_value(block_index idx, ast2_option const& option, expression const& default_value, type_symbol const& option_symbol) -> value_index
        {
            if (option.kind == option_kind::number && default_value.type_is< expression_numeric_literal >())
            {
                return this->create_numeric_literal(default_value.get_as< expression_numeric_literal >().value);
            }
            if (option.kind == option_kind::string && default_value.type_is< expression_string_literal >())
            {
                return this->create_string_literal(default_value.get_as< expression_string_literal >().value);
            }
            if (option.kind == option_kind::boolean && default_value.type_is< expression_value_keyword >())
            {
                auto const& keyword = default_value.get_as< expression_value_keyword >().keyword;
                auto bool_value = option_bool_value(keyword);
                if (bool_value.has_value())
                {
                    return this->create_bool_value(idx, *bool_value);
                }
            }

            throw semantic_compilation_error("Option default value for " + to_string(option_symbol) + " does not match the declared option kind");
        }

        auto co_generate_option_value(block_index idx, type_symbol const& option_symbol, ast2_option const& option, std::set< type_symbol > resolving_options) -> co_type< value_index >
        {
            if (!resolving_options.insert(option_symbol).second)
            {
                throw semantic_compilation_error("Cyclic DEFAULT_FROM while resolving option " + to_string(option_symbol));
            }

            auto options_map = co_await rpnx::querygraph::request< module_options_map_query >(std::monostate{});
            if (auto option_it = options_map.find(option_symbol); option_it != options_map.end())
            {
                co_return this->create_configured_option_value(idx, option, option_it->second, option_symbol);
            }

            if (option.option_default.has_value() && option.option_default->type_is< ast2_option_default_from >())
            {
                auto const& default_from = option.option_default->get_as< ast2_option_default_from >().symbol;
                auto context = type_parent(option_symbol).value_or(option_symbol);
                auto default_option_symbol = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                    .context = context,
                    .type = default_from,
                });
                if (!default_option_symbol.has_value())
                {
                    throw semantic_compilation_error("DEFAULT_FROM option for " + to_string(option_symbol) + " did not resolve: " + to_string(default_from));
                }

                auto default_kind = co_await rpnx::querygraph::request< symbol_type_query >(*default_option_symbol);
                if (default_kind != symbol_kind::option)
                {
                    throw semantic_compilation_error("DEFAULT_FROM for " + to_string(option_symbol) + " resolved to non-option symbol " + to_string(*default_option_symbol));
                }

                auto default_symboid = co_await rpnx::querygraph::request< symboid_query >(*default_option_symbol);
                if (!default_symboid.template type_is< ast2_option >())
                {
                    throw compiler_bug("DEFAULT_FROM option symbol did not resolve to ast2_option");
                }

                auto const& default_option = default_symboid.template get_as< ast2_option >();
                if (default_option.kind != option.kind)
                {
                    throw semantic_compilation_error("DEFAULT_FROM option for " + to_string(option_symbol) + " has a different kind: " + to_string(*default_option_symbol));
                }

                co_return co_await co_generate_option_value(idx, *default_option_symbol, default_option, std::move(resolving_options));
            }
            if (option.option_default.has_value() && option.option_default->type_is< ast2_option_default_value >())
            {
                auto const& default_value = option.option_default->get_as< ast2_option_default_value >().value;
                co_return this->create_default_option_value(idx, option, default_value, option_symbol);
            }

            throw semantic_compilation_error("No configured or default value for option " + to_string(option_symbol));
        }

        auto co_generate_option_value(block_index idx, type_symbol const& option_symbol, ast2_option const& option) -> co_type< value_index >
        {
            co_return co_await co_generate_option_value(idx, option_symbol, option, {});
        }

        auto co_lookup_symbol(block_index idx, type_symbol sym) -> co_type< std::optional< value_index > >
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
                // co_yield rpnx::querygraph::debug_message("lookup {}", name);
                auto lookup = this->local_value_direct_lookup(idx, name);
                if (lookup)
                {
                    co_return this->materialize_lookup_reference(idx, *lookup);
                }
                else
                {
                    if (this->state.scoped_definitions.contains(name))
                    {
                        auto const& def = this->state.scoped_definitions.at(name);
                        if (def.template type_is< scoped_typedef >())
                        {
                            auto def_type = def.template get_as< scoped_typedef >().type;
                            assert(!type_is_contextual(def_type));
                            if (typeis< attached_type_reference >(def_type))
                            {
                                attached_type_reference const& attached = as< attached_type_reference >(def_type);
                                if (!typeis< void_type >(attached.carrying_type))
                                {
                                    throw semantic_compilation_error("Cannot materialize bound attached type without a carrier value: " + to_string(def_type));
                                }
                                auto binding = this->create_binding(value_index(0), attached.attached_symbol);
                                co_return binding;
                            }
                            auto binding = this->create_binding(value_index(0), def_type);
                            co_return binding;
                        }
                        if (def.template type_is< scoped_static >())
                        {
                            auto const& symbol = def.template get_as< scoped_static >().symbol;
                            auto const& input = this->state.statics.at(symbol);
                            if (!input.mutation_result_id.has_value())
                            {
                                std::map< static_local_ref, static_snapshot_ref > remapped;
                                auto snapshot_symbol = this->create_ordinary_snapshot_for_binding(symbol, remapped, false);
                                co_return this->create_antestatal_reference(idx, type_symbol(snapshot_symbol), input.type, false);
                            }
                            co_return this->create_antestatal_reference(idx, type_symbol(symbol), input.type, input.mutation_result_id.has_value());
                        }
                        throw rpnx::unimplemented();
                    }
                    if (auto static_symbol = this->find_visible_static_binding(name); static_symbol.has_value())
                    {
                        auto const& binding = this->state.statics.at(*static_symbol);
                        std::map< static_local_ref, static_snapshot_ref > remapped;
                        auto snapshot_symbol = this->create_ordinary_snapshot_for_binding(*static_symbol, remapped, false);
                        co_return this->create_antestatal_reference(idx, type_symbol(snapshot_symbol), binding.type, false);
                    }
                    if (this->state.packs.contains(name))
                    {
                        throw semantic_compilation_error("Cannot use positional pack '" + name + "' directly; use PACK_SIZE, PACK_ARG, or PACK_ARG_TYPE.");
                    }
                }
            }
            auto canonical_symbol_opt = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = ctx, .type = sym});

            if (!canonical_symbol_opt)
            {
                co_return std::nullopt;
            }

            assert(!type_is_contextual(canonical_symbol_opt.value()));

            auto canonical_symbol = canonical_symbol_opt.value();
            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                co_yield rpnx::querygraph::debug_message("co_lookup_symbol({}) -> {}", symbol_str, quxlang::to_string(canonical_symbol));
            }

            auto kind = co_await rpnx::querygraph::request< symbol_type_query >(canonical_symbol);

            if (kind == quxlang::symbol_kind::option)
            {
                auto option_symboid = co_await rpnx::querygraph::request< symboid_query >(canonical_symbol);
                if (!option_symboid.template type_is< ast2_option >())
                {
                    throw compiler_bug("Option symbol did not resolve to ast2_option");
                }
                co_return co_await co_generate_option_value(idx, canonical_symbol, option_symboid.template get_as< ast2_option >());
            }

            if (kind == quxlang::symbol_kind::enum_value || kind == quxlang::symbol_kind::flagset_value)
            {
                type_symbol const parent_type = type_parent(canonical_symbol).value();
                std::string const value_name = typeis< subsymbol >(canonical_symbol) ? as< subsymbol >(canonical_symbol).name : as< submember >(canonical_symbol).name;
                std::uint64_t numeric_value = 0;
                if (kind == quxlang::symbol_kind::enum_value)
                {
                    enum_info const info = co_await rpnx::querygraph::request< enum_info_query >(parent_type);
                    bool found = false;
                    for (enum_value_info const& value : info.values)
                    {
                        if (value.name == value_name)
                        {
                            numeric_value = value.value;
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        throw compiler_bug("enum value symbol did not appear in enum_info: " + to_string(canonical_symbol));
                    }
                }
                else
                {
                    flagset_info const info = co_await rpnx::querygraph::request< flagset_info_query >(parent_type);
                    bool found = false;
                    for (flagset_value_info const& value : info.values)
                    {
                        if (value.name == value_name)
                        {
                            numeric_value = value.mask;
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        throw compiler_bug("flagset value symbol did not appear in flagset_info: " + to_string(canonical_symbol));
                    }
                }

                value_index value = this->create_local_value(parent_type);
                vmir2::load_const_int instr;
                instr.target = get_local_index(value);
                instr.value = std::to_string(numeric_value);
                this->emit(idx, instr);
                co_return value;
            }

            value_index index(0);

            auto binding = this->create_binding(value_index(0), canonical_symbol);

            if (kind == quxlang::symbol_kind::global_variable)
            {
                auto global_get_reference = submember{.of = canonical_symbol, .name = "GET_REFERENCE"};
                index = co_await this->co_gen_call_functum(idx, global_get_reference, {});
            }
            else if (kind == quxlang::symbol_kind::implementation_)
            {
                subsymbol implementation_get_interface = subsymbol{.of = canonical_symbol, .name = "GET_INTERFACE_IMPL"};
                index = co_await this->co_gen_call_functum(idx, implementation_get_interface, {});
            }
            else
            {
                index = binding;
            }

            co_return index;
        }

        auto co_gen_call_ctor(block_index& bidx, type_symbol new_type, codegen_invocation_args args) -> co_type< value_index >
        {
            auto ctor = submember{.of = new_type, .name = "CONSTRUCTOR"};
            auto new_object = create_local_value(new_type);
            args.named["THIS"] = new_object;
            auto retval = co_await co_gen_call_functum(bidx, ctor, args);

            assert(retval == 0);

            auto dtor = co_await rpnx::querygraph::request< class_default_dtor_query >(new_type);
            if (dtor)
            {
                this->add_nontrivial_default_dtor(new_type, *dtor);
            }
            co_return new_object;
        }

        auto codegen_args_to_invotype(block_index& bidx, codegen_invocation_args const& args) -> invotype
        {
            invotype calltype;

            for (auto const& arg : args.positional)
            {
                calltype.positional.push_back(this->current_type(bidx, arg));
            }

            for (auto const& [name, arg] : args.named)
            {
                type_symbol arg_type = this->current_type(bidx, arg);
                if (name == "THIS" && typeis< attached_type_reference >(arg_type))
                {
                    arg_type = as< attached_type_reference >(arg_type).carrying_type;
                }
                calltype.named[name] = std::move(arg_type);
            }

            return calltype;
        }

        auto co_try_gen_call_ctor_with_named_argument(block_index& bidx, type_symbol new_type, std::string const& arg_name, value_index arg_val) -> co_type< std::optional< value_index > >
        {
            auto ctor = submember{.of = new_type, .name = "CONSTRUCTOR"};

            codegen_invocation_args args;
            args.named[arg_name] = arg_val;

            invotype calltype;
            calltype.named[arg_name] = this->current_type(bidx, arg_val);
            calltype.named["THIS"] = create_nslot(new_type);

            auto instanciation = co_await rpnx::querygraph::request< instanciation_query >(initialization_reference{
                .initializee = ctor,
                .parameters = instatype_from_invotype(calltype),
                .adaptations = allowed_adaptations::destination_rebinding,
            });

            if (!instanciation.has_value())
            {
                co_return std::nullopt;
            }

            auto new_object = create_local_value(new_type);
            args.named["THIS"] = new_object;
            auto retval = co_await this->co_gen_call_functanoid(bidx, *instanciation, args, allowed_adaptations::destination_rebinding);

            assert(retval == 0);

            auto dtor = co_await rpnx::querygraph::request< class_default_dtor_query >(new_type);
            if (dtor)
            {
                this->add_nontrivial_default_dtor(new_type, *dtor);
            }

            co_return new_object;
        }

        auto co_generate(block_index& bidx, expression_char_literal chr) -> co_type< value_index >
        {
            auto number_string = std::to_string(static_cast< int >(chr.value));
            auto val = this->create_numeric_literal(number_string);
            assert(val != 0);
            auto val_type = this->current_type(bidx, val);
            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                co_yield rpnx::querygraph::debug_message("Generated numeric literal from char {} of type {}", static_cast< std::uint64_t >(val), to_string(val_type));
            }
            co_return val;
        }

        auto co_generate(block_index& bidx, expression_call call) -> co_type< value_index >
        {
            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                co_yield rpnx::querygraph::debug_message("gen_call_expr A()");
            }
            auto callee = co_await co_generate_expr(bidx, call.callee);

            type_symbol callee_type = this->current_type(bidx, callee);
            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                co_yield rpnx::querygraph::debug_message("gen_call_expr B() -> callee_type={}", quxlang::to_string(callee_type));
            }

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
            symbol_kind attached_symbol_kind = co_await rpnx::querygraph::request< symbol_type_query >(attached_symbol);

            codegen_invocation_args args;
            std::string callee_type_string2 = to_string(as< attached_type_reference >(callee_type));

            // co_yield rpnx::querygraph::debug_message("requesting generate call to bindval={} bindsym={}", to_string(carrying_type), to_string(attached_symbol));

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
                value_index this_value = callee_binding.bound_value;
                if (is_ref(this->current_type(bidx, this_value)))
                {
                    this_value = this->copy_ref_value(bidx, this_value);
                }
                args.named["THIS"] = this_value;
            }

            if (attached_symbol_kind == symbol_kind::class_)
            {
                if (!typeis< void_type >(as< attached_type_reference >(callee_type).carrying_type))
                {
                    throw compiler_bug("attached class reference unexpectedly carried a value type");
                }
                auto object_type = as< attached_type_reference >(callee_type).attached_symbol;

                auto val = co_await co_gen_call_ctor(bidx, object_type, args);

                co_return val;
            }

            co_return co_await co_gen_call_functum(bidx, as< attached_type_reference >(callee_type).attached_symbol, args);
        }

        auto co_generate(block_index& bidx, expression_lambda const& lambda) -> co_type< value_index >
        {
            std::size_t lambda_index = this->state.next_lambda_index++;
            auto possible_captures = this->build_lambda_possible_captures(bidx);
            auto dry_context = this->lambda_static_context_from_current();
            auto dry_run = co_await this->co_analyze_lambda_captures(lambda, possible_captures, std::move(dry_context));
            auto operator_declaration = this->make_lambda_operator_declaration(lambda);
            co_await this->co_publish_lambda_subqueries(lambda_index, possible_captures, dry_run, std::move(operator_declaration));

            type_symbol closure_type = make_lambda_closure_symbol(this->ctx, lambda_index);
            value_index closure = this->create_local_value(closure_type);

            codegen_invocation_args ctor_args;
            ctor_args.named["THIS"] = closure;
            for (std::size_t i = 0; i < dry_run.captures.size(); i++)
            {
                lambda_capture_selection const& capture = dry_run.captures.at(i);
                std::optional< value_index > source = co_await this->co_lookup_symbol(bidx, freebound_identifier{.name = capture.name});
                if (!source.has_value())
                {
                    throw compiler_bug("Lambda capture disappeared during closure generation: " + capture.name);
                }
                ctor_args.positional.push_back(*source);
            }

            type_symbol constructor = submember{.of = closure_type, .name = "CONSTRUCTOR"};
            co_await this->co_gen_call_functum(bidx, std::move(constructor), std::move(ctor_args), allowed_adaptations::source_rebinding);

            co_return closure;
        }

      public:
        auto co_gen_defer_dtor(block_index& bidx, value_index val, type_symbol dtor, codegen_invocation_args args) -> co_type< void >
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

        auto co_gen_inline_array_positional_ctor(block_index& current_block, instanciation_reference const& func, codegen_invocation_args const& args) -> co_type< void >
        {
            if (!typeis< submember >(func.temploid.templexoid))
            {
                throw compiler_bug("Expected array constructor functum to be a submember");
            }

            submember const& member = as< submember >(func.temploid.templexoid);
            if (member.name != "CONSTRUCTOR" || !typeis< array_type >(member.of))
            {
                throw compiler_bug("Expected array constructor functum");
            }

            array_type const& array = as< array_type >(member.of);
            type_symbol element_type = array.element_type;
            if (!array.element_count.type_is< expression_numeric_literal >())
            {
                throw semantic_compilation_error("Array size must be a numeric literal");
            }

            std::string const& array_size = as< expression_numeric_literal >(array.element_count).value;
            auto ule = bytemath::detail::string_to_le_raw(array_size);
            bytemath::fixed_int_options opts;
            opts.bits = 64;
            opts.has_sign = false;
            opts.overflow_undefined = true;
            auto [res, ok] = bytemath::unlimited_to_int< std::uint64_t >(opts, ule);
            if (!ok)
            {
                throw semantic_compilation_error("Array size is too large");
            }

            if (args.positional.size() != static_cast< std::size_t >(res))
            {
                throw semantic_compilation_error("Array positional constructor argument count does not match array length");
            }

            array_initializer_type init_type;
            init_type.count = res;
            init_type.element_type = element_type;

            value_index initializer = create_local_value(init_type);
            this->emit(current_block, vmir2::array_init_start{.on_value = get_local_index(args.named.at("THIS")), .initializer = get_local_index(initializer)});

            type_symbol constructor = submember{.of = element_type, .name = "CONSTRUCTOR"};
            for (value_index argument : args.positional)
            {
                value_index element = this->create_local_value(element_type);
                this->emit(current_block, vmir2::array_init_element{.initializer = get_local_index(initializer), .target = get_local_index(element)});

                codegen_invocation_args element_args;
                element_args.named["THIS"] = element;
                element_args.named["OTHER"] = argument;
                co_await this->co_gen_call_functum(current_block, constructor, element_args);
            }

            this->emit(current_block, vmir2::array_init_finish{.initializer = get_local_index(initializer)});
            co_return;
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

        auto co_gen_call_functanoid(block_index& bidx, instanciation_reference what, codegen_invocation_args expression_args, allowed_adaptations adaptations) -> co_type< value_index >
        {
            auto call_args_types = co_await rpnx::querygraph::request< instanciation_concrete_params_query >(what);

            codegen_invocation_args invocation_args;
            auto function_decl_opt = co_await rpnx::querygraph::request< function_declaration_query >(what.temploid);

            auto create_arg_value = [&](std::string const& arg_name, value_index arg_expr_index, type_symbol arg_target_type) -> co_type< value_index >
            {
                // TODO: Support PRValue args
                auto arg_expr_type = this->current_type(bidx, arg_expr_index);
                bool arg_alive = this->value_alive(bidx, arg_expr_index);

                if (arg_name == "THIS" && typeis< attached_type_reference >(arg_expr_type) && !typeis< attached_type_reference >(arg_target_type))
                {
                    if (!this->state.genvalues.at(arg_expr_index).template type_is< codegen_binding >())
                    {
                        throw semantic_compilation_error("Expected attached THIS argument to be a binding");
                    }

                    attached_type_reference const& attached = as< attached_type_reference >(arg_expr_type);
                    if (attached.carrying_type != arg_target_type)
                    {
                        throw semantic_compilation_error("Cannot lower attached THIS argument " + to_string(arg_expr_type) + " to " + to_string(arg_target_type));
                    }

                    codegen_binding const& binding = this->state.genvalues.at(arg_expr_index).template get_as< codegen_binding >();
                    value_index this_value = binding.bound_value;
                    if (is_ref(this->current_type(bidx, this_value)))
                    {
                        this_value = this->copy_ref_value(bidx, this_value);
                    }
                    co_return this_value;
                }

                if (!arg_alive)
                {
                    assert(!is_ref(arg_expr_type));
                    // arg_expr_type = nvalue_slot{arg_expr_type};
                }

                if (arg_expr_type == arg_target_type)
                {
                    if (typeis< attached_type_reference >(arg_target_type))
                    {
                        co_return co_await this->co_copy_attached_binding(bidx, arg_expr_index, arg_target_type);
                    }
                    co_return arg_expr_index;
                }

                co_return co_await co_gen_argument_adaptation(bidx, arg_expr_index, arg_target_type, adaptations);
            };

            auto generate_default_expr = [&](expression const& expr) -> co_type< value_index >
            {
                auto declaration_context = type_parent(what.temploid.templexoid).value_or(void_type{});
                auto context_scope = this->scoped_declaration_context(bidx, std::move(declaration_context));
                co_await this->co_apply_context_instantiation_scopes(what);
                co_return co_await this->co_generate_expr(bidx, expr);
            };

            std::map< std::string, expression const* > named_defaults;
            std::vector< expression const* > positional_defaults;
            if (function_decl_opt.has_value())
            {
                for (auto const& param : function_decl_opt->header.call_parameters)
                {
                    if (param.api_name.has_value())
                    {
                        if (param.default_expr.has_value())
                        {
                            named_defaults[param.api_name.value()] = &*param.default_expr;
                        }
                        continue;
                    }
                    positional_defaults.push_back(param.default_expr.has_value() ? &*param.default_expr : nullptr);
                }
            }

            for (auto const& [name, arg_accepted_type] : call_args_types.named)
            {
                value_index arg_expr_index(0);
                if (auto it = expression_args.named.find(name); it != expression_args.named.end())
                {
                    arg_expr_index = it->second;
                }
                else
                {
                    auto default_it = named_defaults.find(name);
                    if (default_it == named_defaults.end())
                    {
                        throw semantic_compilation_error("Missing argument @" + name + " and no default expression is available");
                    }
                    arg_expr_index = co_await generate_default_expr(*default_it->second);
                }

                auto arg_index = co_await create_arg_value(name, arg_expr_index, parameter_instantiation_type(arg_accepted_type));

                invocation_args.named[name] = arg_index;
            }

            for (std::size_t i = 0; i < call_args_types.positional.size(); i++)
            {
                auto arg_accepted_type = parameter_instantiation_type(call_args_types.positional.at(i));

                value_index arg_expr_index(0);
                if (i < expression_args.positional.size())
                {
                    arg_expr_index = expression_args.positional.at(i);
                }
                else
                {
                    if (i >= positional_defaults.size() || positional_defaults.at(i) == nullptr)
                    {
                        throw semantic_compilation_error("Missing positional argument and no default expression is available");
                    }
                    arg_expr_index = co_await generate_default_expr(*positional_defaults.at(i));
                }

                auto arg_index = co_await create_arg_value(std::string{}, arg_expr_index, arg_accepted_type);
                invocation_args.positional.push_back(arg_index);
            }

            assert(!type_is_contextual(what));
            auto return_type = co_await rpnx::querygraph::request< functanoid_return_type_query >(what);

            // Index 0 is defined to be the special "void" value.
            value_index retval(0);

            if (!typeis< void_type >(return_type))
            {
                auto return_slot = create_local_value(return_type);
                // co_yield rpnx::querygraph::debug_message("Created return slot {}", return_slot);

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

        auto co_gen_reinterpret_reference(block_index& bidx, value_index ref_index, type_symbol target_ref_type) -> co_type< value_index >
        {
            auto ref_type = this->current_type(bidx, ref_index);

            std::string ref_type_str = to_string(ref_type);
            std::string target_ref_type_str = to_string(target_ref_type);

            if (!is_ref(target_ref_type) || !is_ref(ref_type))
            {
                throw semantic_compilation_error("Cannot gen_reinterpret_reference reinterpret non-reference types");
            }

            auto new_index = this->create_local_value(target_ref_type);

            vmir2::cast_ptrref csr;
            csr.source_index = get_local_index(ref_index);
            csr.target_index = get_local_index(new_index);

            this->emit(bidx, csr);

            co_return new_index;
        }

        auto co_gen_reference_conversion(block_index& bidx, value_index vidx, type_symbol target_reference_type) -> co_type< value_index >
        {
            // TODO: Support dynamic/static casts
            co_return co_await co_gen_reinterpret_reference(bidx, vidx, target_reference_type);
        }

        auto co_gen_value_conversion(block_index& bidx, value_index vidx, type_symbol target_value_type) -> co_type< value_index >
        {
            // TODO: support conversion other than via constructor.
            co_return co_await co_gen_value_constructor_conversion(bidx, vidx, target_value_type);
        }

        auto co_gen_value_constructor_conversion(block_index& bidx, value_index vidx, type_symbol target_value_type) -> co_type< value_index >
        {
            type_symbol value_type = this->current_type(bidx, vidx);
            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                co_yield rpnx::querygraph::debug_message("co_gen_value_conversion({}({}), {})", static_cast< std::uint64_t >(vidx), to_string(value_type), to_string(target_value_type));
            }

            co_return co_await co_gen_argument_adaptation(bidx, vidx, target_value_type, allowed_adaptations::destination_rebinding);
        }

        auto co_gen_implicit_conversion(block_index& bidx, value_index vidx, type_symbol target_type, std::optional< value_index > constructed_index = std::nullopt) -> co_type< value_index >
        {
            type_symbol value_type = this->current_type(bidx, vidx);
            // co_yield rpnx::querygraph::debug_message("gen_implicit_conversion({}({}), {})", vidx, to_string(value_type), to_string(target_type));

            if (value_type == target_type)
            {
                assert(vidx != value_index(0));
                co_return vidx;
            }

            co_return co_await co_gen_argument_adaptation(bidx, vidx, target_type, allowed_adaptations::destination_rebinding);
        }

        /**
         * @pre The invocation argument shape is an existing, alread
         * @param bidx
         * @param what
         * @param args
         * @return
         */
        auto co_gen_invoke_builtin(block_index& bidx, instanciation_reference what, codegen_invocation_args const& args) -> co_type< void >
        {
            /// THIS IS THE MAIN BIND POINT FOR NEW INSTRUCTIONS AND BUILTIN TYPES
            /// DO NOT GENERATE NEW FUNCTIONS THAT ONLY OUTPUT ONE INSTRUCTION THEN RETURN
            instatype concrete_params = co_await rpnx::querygraph::request< instanciation_concrete_params_query >(what);
            invotype concrete_call = invotype_from_instatype(concrete_params);
            builtin_function_kind const builtin_kind = co_await rpnx::querygraph::request< function_builtin_query >(what.temploid);

            if (builtin_kind == builtin_function_kind::not_builtin)
            {
                throw compiler_bug("Expected builtin functanoid invocation, got non-builtin: " + to_string(what));
            }

            if (builtin_kind == builtin_function_kind::builtin_special)
            {
                if (co_await this->co_try_emit_interface_builtin(bidx, what, concrete_call, args))
                {
                    co_return;
                }
                if (co_await this->co_try_emit_nominal_integer_builtin(bidx, what, concrete_call, args))
                {
                    co_return;
                }
                if (typeis< submember >(what.temploid.templexoid))
                {
                    submember const& member = as< submember >(what.temploid.templexoid);
                    if (member.name == "CONSTRUCTOR" && typeis< array_type >(member.of) && !args.positional.empty())
                    {
                        co_await this->co_gen_inline_array_positional_ctor(bidx, what, args);
                        co_return;
                    }
                }
            }
            else if (builtin_kind == builtin_function_kind::builtin_intrinsic)
            {
                auto intrinsic = this->intrinsic_instruction(what, concrete_call, args);
                if (!intrinsic.has_value())
                {
                    throw compiler_bug("Builtin intrinsic was not lowered to a VMIR instruction: " + to_string(what));
                }
                this->emit(bidx, intrinsic.value());
                co_return;
            }

            // Generated routines are for COMPLEX operations only
            // such as builtin SERIALIZE/DESERIALIZE operations that
            // don't make sense to implement with a single instruction.

            std::string what_str = to_string(what);
            // std::string args_str = to_string(args);
            vmir2::invoke ivk;
            ivk.what = what;
            ivk.args = get_invocation_args(concrete_params, args);

            this->emit(bidx, ivk);

            co_return;
        }

        auto interface_slot_key_from_functanoid(instanciation_reference const& what) -> co_type< interface_slot_key >
        {
            interface_slot_key key;
            if (!typeis< submember >(what.temploid.templexoid))
            {
                throw compiler_bug("Interface slot key requires a member functanoid");
            }
            submember const& member = as< submember >(what.temploid.templexoid);
            key.name = member.name;
            auto concrete_params = co_await rpnx::querygraph::request< instanciation_concrete_params_query >(what);
            key.concrete_params = invotype_from_instatype(concrete_params);
            key.concrete_params.named.erase("THIS");

            auto return_type = co_await rpnx::querygraph::request< functanoid_return_type_query >(what);
            if (!typeis< void_type >(return_type))
            {
                key.concrete_return_type = std::move(return_type);
            }
            co_return key;
        }

        auto interface_slot_has_default_body(type_symbol interface_type, interface_slot_key const& key) -> co_type< bool >
        {
            std::vector< interface_slot > slots = co_await rpnx::querygraph::request< interface_slot_list_query >(std::move(interface_type));
            for (interface_slot const& slot : slots)
            {
                if (slot.key == key)
                {
                    co_return slot.declaration.has_default_body;
                }
            }
            throw compiler_bug("Interface slot not found after overload resolution");
        }

        auto co_try_emit_interface_builtin(block_index& bidx, instanciation_reference const& what, invotype const&, codegen_invocation_args const& args) -> co_type< bool >
        {
            if (!typeis< submember >(what.temploid.templexoid))
            {
                co_return false;
            }

            submember const& member = as< submember >(what.temploid.templexoid);
            symbol_kind parent_kind = co_await rpnx::querygraph::request< symbol_type_query >(member.of);
            if (parent_kind != symbol_kind::interface_)
            {
                co_return false;
            }
            if (member.name == "CONSTRUCTOR")
            {
                if (args.named.contains("THIS") && args.size() == 1)
                {
                    this->emit(bidx, vmir2::interface_init{
                                         .target = get_local_index(args.named.at("THIS")),
                                         .interface_type = member.of,
                                         .is_default = true,
                                     });
                    co_return true;
                }

                if (args.named.contains("THIS") && args.named.contains("OTHER") && args.size() == 2)
                {
                    this->emit(bidx, vmir2::load_from_ref{
                                         .from_reference = get_local_index(args.named.at("OTHER")),
                                         .to_value = get_local_index(args.named.at("THIS")),
                                     });
                    co_return true;
                }
            }

            if (member.name == "OPERATOR:=" && args.named.contains("THIS") && args.named.contains("OTHER") && args.size() == 2)
            {
                this->emit(bidx, vmir2::store_to_ref{
                                     .from_value = get_local_index(args.named.at("OTHER")),
                                     .to_reference = get_local_index(args.named.at("THIS")),
                                 });
                co_return true;
            }

            if (member.name == "OPERATOR??" && args.named.contains("THIS") && args.named.contains("RETURN") && args.size() == 2)
            {
                value_index default_check = this->create_local_value(bool_type{});
                this->emit(bidx, vmir2::interface_is_default{
                                     .interface_value = get_local_index(args.named.at("THIS")),
                                     .result = get_local_index(default_check),
                                 });
                this->emit(bidx, vmir2::to_bool_not{
                                     .from = get_local_index(default_check),
                                     .to = get_local_index(args.named.at("RETURN")),
                });
                co_return true;
            }
            if (member.name == "OPERATOR?!" && args.named.contains("THIS") && args.named.contains("RETURN") && args.size() == 2)
            {
                this->emit(bidx, vmir2::interface_is_default{
                                     .interface_value = get_local_index(args.named.at("THIS")),
                                     .result = get_local_index(args.named.at("RETURN")),
                                 });
                co_return true;
            }
            co_return false;
        }

        auto co_try_emit_interface_builtin_from_locals(block_index& bidx, instanciation_reference const& what) -> co_type< bool >
        {
            codegen_invocation_args args;
            std::optional< value_index > this_value = this->local_value_direct_lookup(bidx, "THIS");
            if (this_value.has_value())
            {
                args.named["THIS"] = *this_value;
            }
            std::optional< value_index > other_value = this->local_value_direct_lookup(bidx, "OTHER");
            if (other_value.has_value())
            {
                args.named["OTHER"] = *other_value;
            }
            std::optional< value_index > return_value = this->local_value_direct_lookup(bidx, "RETURN");
            if (return_value.has_value())
            {
                args.named["RETURN"] = *return_value;
            }

            instatype concrete_params = co_await rpnx::querygraph::request< instanciation_concrete_params_query >(what);
            invotype concrete_call = invotype_from_instatype(concrete_params);
            co_return co_await this->co_try_emit_interface_builtin(bidx, what, concrete_call, args);
        }

        auto co_try_emit_nominal_integer_builtin(block_index& bidx, instanciation_reference const& what, invotype const& call, codegen_invocation_args const& args) -> co_type< bool >
        {
            if (!typeis< submember >(what.temploid.templexoid))
            {
                co_return false;
            }

            submember const& member = as< submember >(what.temploid.templexoid);
            symbol_kind const parent_kind = co_await rpnx::querygraph::request< symbol_type_query >(member.of);
            if (parent_kind != symbol_kind::enum_ && parent_kind != symbol_kind::flagset_)
            {
                co_return false;
            }
            auto emit_load_const_u64 = [&](value_index target, std::uint64_t value) -> void
            {
                vmir2::load_const_int instr;
                instr.target = get_local_index(target);
                instr.value = std::to_string(value);
                this->emit(bidx, instr);
            };

            if (member.name == "CONSTRUCTOR" && args.named.contains("THIS") && args.size() == 1)
            {
                if (parent_kind == symbol_kind::flagset_)
                {
                    this->emit(bidx, vmir2::load_const_zero{.target = get_local_index(args.named.at("THIS"))});
                    co_return true;
                }

                enum_info const info = co_await rpnx::querygraph::request< enum_info_query >(member.of);
                if (!info.default_value_name.has_value())
                {
                    co_return false;
                }
                for (enum_value_info const& value : info.values)
                {
                    if (value.name == *info.default_value_name)
                    {
                        emit_load_const_u64(args.named.at("THIS"), value.value);
                        co_return true;
                    }
                }
                throw compiler_bug("ENUM default value was not present in enum_info");
            }

            if (member.name == "CONSTRUCTOR" && args.named.contains("THIS") && args.named.contains("OTHER") && args.size() == 2)
            {
                this->emit(bidx, vmir2::load_from_ref{.from_reference = get_local_index(args.named.at("OTHER")), .to_value = get_local_index(args.named.at("THIS"))});
                co_return true;
            }

            if (member.name == "CONSTRUCTOR" && parent_kind == symbol_kind::flagset_ && args.named.contains("THIS") && args.named.contains("EXPLICIT") && args.size() == 2)
            {
                this->emit(bidx, vmir2::iconv{.from = get_local_index(args.named.at("EXPLICIT")), .to = get_local_index(args.named.at("THIS")), .convtype = vmir2::conversion_class::partial});
                co_return true;
            }

            if (member.name == "OPERATOR:=" && args.named.contains("THIS") && args.named.contains("OTHER") && args.size() == 2)
            {
                this->emit(bidx, vmir2::store_to_ref{.from_value = get_local_index(args.named.at("OTHER")), .to_reference = get_local_index(args.named.at("THIS"))});
                co_return true;
            }

            if ((member.name == "OPERATOR??" || member.name == "OPERATOR?!") && args.named.contains("THIS") && args.named.contains("RETURN") && args.size() == 2)
            {
                if (member.name == "OPERATOR??")
                {
                    this->emit(bidx, vmir2::to_bool{.from = get_local_index(args.named.at("THIS")), .to = get_local_index(args.named.at("RETURN"))});
                }
                else
                {
                    this->emit(bidx, vmir2::to_bool_not{.from = get_local_index(args.named.at("THIS")), .to = get_local_index(args.named.at("RETURN"))});
                }
                co_return true;
            }

            if (args.named.contains("THIS") && args.named.contains("OTHER") && args.named.contains("RETURN") && args.size() == 3)
            {
                std::optional< vmir2::vm_instruction > instr;
                if (implement_binary_instruction< vmir2::cmp_eq >(instr, "==", true, member, call, args))
                {
                    this->emit(bidx, *instr);
                    co_return true;
                }
                if (implement_binary_instruction< vmir2::cmp_ne >(instr, "!=", true, member, call, args))
                {
                    this->emit(bidx, *instr);
                    co_return true;
                }
                if (implement_binary_instruction< vmir2::cmp_lt >(instr, "<", true, member, call, args))
                {
                    this->emit(bidx, *instr);
                    co_return true;
                }
                if (implement_binary_instruction< vmir2::cmp_lt >(instr, ">", true, member, call, args, true))
                {
                    this->emit(bidx, *instr);
                    co_return true;
                }
                if (implement_binary_instruction< vmir2::cmp_ge >(instr, "<=", true, member, call, args, true))
                {
                    this->emit(bidx, *instr);
                    co_return true;
                }
                if (implement_binary_instruction< vmir2::cmp_ge >(instr, ">=", true, member, call, args))
                {
                    this->emit(bidx, *instr);
                    co_return true;
                }

                if (parent_kind == symbol_kind::flagset_)
                {
                    if (implement_binary_instruction< vmir2::bitwise_and >(instr, "#&&", true, member, call, args))
                    {
                        this->emit(bidx, *instr);
                        co_return true;
                    }
                    if (implement_binary_instruction< vmir2::bitwise_or >(instr, "#||", true, member, call, args))
                    {
                        this->emit(bidx, *instr);
                        co_return true;
                    }
                    if (implement_binary_instruction< vmir2::bitwise_xor >(instr, "#^^", true, member, call, args))
                    {
                        this->emit(bidx, *instr);
                        co_return true;
                    }
                    if (implement_binary_instruction< vmir2::bitwise_nand >(instr, "#&!", true, member, call, args))
                    {
                        this->emit(bidx, *instr);
                        co_return true;
                    }
                    if (implement_binary_instruction< vmir2::bitwise_nor >(instr, "#|!", true, member, call, args))
                    {
                        this->emit(bidx, *instr);
                        co_return true;
                    }
                    if (implement_binary_instruction< vmir2::bitwise_nxor >(instr, "#^!", true, member, call, args))
                    {
                        this->emit(bidx, *instr);
                        co_return true;
                    }
                    if (implement_binary_instruction< vmir2::bitwise_implies >(instr, "#^>", true, member, call, args))
                    {
                        this->emit(bidx, *instr);
                        co_return true;
                    }
                    if (implement_binary_instruction< vmir2::bitwise_implied >(instr, "#^<", true, member, call, args))
                    {
                        this->emit(bidx, *instr);
                        co_return true;
                    }
                }
            }

            if (parent_kind == symbol_kind::flagset_ && args.named.contains("THIS") && args.named.contains("RETURN") && args.size() == 2 && member.name == "OPERATOR#!!")
            {
                this->emit(bidx, vmir2::bitwise_inverse{.value = get_local_index(args.named.at("THIS")), .result = get_local_index(args.named.at("RETURN"))});
                co_return true;
            }

            if (parent_kind == symbol_kind::flagset_ && args.named.contains("THIS") && args.named.contains("OTHER") && args.size() == 2)
            {
                std::optional< vmir2::vm_instruction > instr;
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_and >(instr, "#&&=", member, call, args))
                {
                    this->emit(bidx, *instr);
                    co_return true;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_or >(instr, "#||=", member, call, args))
                {
                    this->emit(bidx, *instr);
                    co_return true;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_xor >(instr, "#^^=", member, call, args))
                {
                    this->emit(bidx, *instr);
                    co_return true;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_nand >(instr, "#&!=", member, call, args))
                {
                    this->emit(bidx, *instr);
                    co_return true;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_nor >(instr, "#|!=", member, call, args))
                {
                    this->emit(bidx, *instr);
                    co_return true;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_nxor >(instr, "#^!=", member, call, args))
                {
                    this->emit(bidx, *instr);
                    co_return true;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_implies >(instr, "#^>=", member, call, args))
                {
                    this->emit(bidx, *instr);
                    co_return true;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_implied >(instr, "#^<=", member, call, args))
                {
                    this->emit(bidx, *instr);
                    co_return true;
                }
            }

            co_return false;
        }

        auto co_generate_interface_builtin(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();
            block_index current_block = block_index(0);

            if (!co_await this->co_try_emit_interface_builtin_from_locals(current_block, func))
            {
                throw compiler_bug("Interface builtin routine is not implemented: " + quxlang::to_string(func));
            }

            co_await co_generate_builtin_return(current_block);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        bool is_intrinsic_type(type_symbol of_type)
        {
            return of_type.type_is< int_type >() || of_type.type_is< float_type >() || of_type.type_is< bool_type >() || of_type.type_is< procedure_type >() || of_type.type_is< ptrref_type >() || of_type.type_is< array_type >() || of_type.type_is< byte_type >() || of_type.type_is< readonly_constant >() || of_type.type_is< constexpr_proxy >() || is_atomic_type(of_type);
        }

        /// Converts a canonical atomic access-mode type into a VMIR access mode.
        static auto intrinsic_atomic_mode_from_type(type_symbol const& type) -> std::optional< atomic_access_mode >
        {
            if (!typeis< builtin_symbol >(type))
            {
                return std::nullopt;
            }
            return atomic_access_mode_from_name(as< builtin_symbol >(type).name);
        }

        /// Reads the single shorthand atomic mode parameter from an atomic operation instantiation.
        static auto single_atomic_mode_from_instanciation(instanciation_reference const& instanciation) -> atomic_access_mode
        {
            auto mode_arg = instanciation.params.named.find("T");
            if (mode_arg == instanciation.params.named.end() || instanciation.params.named.size() != 1 || !instanciation.params.positional.empty())
            {
                throw compiler_bug("atomic intrinsic expects one instantiated access mode");
            }

            std::optional< atomic_access_mode > mode = intrinsic_atomic_mode_from_type(parameter_instantiation_type(mode_arg->second));
            if (!mode.has_value())
            {
                throw compiler_bug("atomic intrinsic received an invalid access mode");
            }
            return *mode;
        }

        /// Reads the success and failure access modes from an atomic compare-exchange instantiation.
        static auto compare_exchange_atomic_modes_from_instanciation(instanciation_reference const& instanciation) -> std::pair< atomic_access_mode, atomic_access_mode >
        {
            auto success_arg = instanciation.params.named.find("SUCCESS");
            auto failure_arg = instanciation.params.named.find("FAILURE");
            if (success_arg == instanciation.params.named.end() || failure_arg == instanciation.params.named.end() || instanciation.params.named.size() != 2 || !instanciation.params.positional.empty())
            {
                throw compiler_bug("atomic compare_exchange intrinsic expects SUCCESS and FAILURE access modes");
            }

            std::optional< atomic_access_mode > success_mode = intrinsic_atomic_mode_from_type(parameter_instantiation_type(success_arg->second));
            std::optional< atomic_access_mode > failure_mode = intrinsic_atomic_mode_from_type(parameter_instantiation_type(failure_arg->second));
            if (!success_mode.has_value() || !failure_mode.has_value())
            {
                throw compiler_bug("atomic compare_exchange intrinsic received an invalid access mode");
            }
            return {*success_mode, *failure_mode};
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

        template < typename Inst >
        bool implement_mut_binary_instruction(std::optional< vmir2::vm_instruction >& out, std::string const& operator_str, submember const& member, invotype const& call, codegen_invocation_args const& args)
        {
            if (member.name != "OPERATOR" + operator_str)
            {
                return false;
            }
            if (!call.named.contains("THIS") || !call.named.contains("OTHER") || args.size() != 2)
            {
                return false;
            }

            Inst instr{};
            instr.target = get_local_index(args.named.at("THIS"));
            instr.value = get_local_index(args.named.at("OTHER"));
            out = instr;
            return true;
        }

        template < typename Inst >
        bool implement_mut_shift_instruction(std::optional< vmir2::vm_instruction >& out, std::string const& operator_str, submember const& member, invotype const& call, codegen_invocation_args const& args)
        {
            if (member.name != "OPERATOR" + operator_str)
            {
                return false;
            }
            if (!call.named.contains("THIS") || !call.named.contains("OTHER") || args.size() != 2)
            {
                return false;
            }

            Inst instr{};
            instr.target = get_local_index(args.named.at("THIS"));
            instr.amount = get_local_index(args.named.at("OTHER"));
            out = instr;
            return true;
        }

        std::optional< vmir2::vm_instruction > intrinsic_instruction(type_symbol func, invotype const& call, codegen_invocation_args args)
        {
            std::string funcname = to_string(func);

            {
                auto instanciation = func.cast_ptr< instanciation_reference >();
                auto allocator_functum = instanciation == nullptr ? nullptr : instanciation->temploid.templexoid.cast_ptr< instanciation_reference >();
                auto builtin = allocator_functum == nullptr ? nullptr : allocator_functum->temploid.templexoid.cast_ptr< builtin_symbol >();
                auto allocator_kind = builtin == nullptr ? std::optional< builtin_allocator_kind >{} : builtin_allocator_kind_from_name(builtin->name);
                if (allocator_kind.has_value())
                {
                    auto type_argument = allocator_functum->params.named.find("T");
                    if (type_argument == allocator_functum->params.named.end() || allocator_functum->params.named.size() != 1 || !allocator_functum->params.positional.empty())
                    {
                        throw compiler_bug("constexpr allocator builtin intrinsic expects one instantiated @T type parameter");
                    }

                    auto const allocated_type = parameter_instantiation_type(type_argument->second);
                    auto const storage_type = type_symbol(storage{.storable_types = {allocated_type}});

                    switch (*allocator_kind)
                    {
                    case builtin_allocator_kind::constexpr_alloc:
                        if (!args.named.contains("RETURN") || args.size() != 1)
                        {
                            throw compiler_bug("CONSTEXPR_ALLOC intrinsic expects only a RETURN slot");
                        }
                        return vmir2::constexpr_alloc{
                            .storage_type = storage_type,
                            .result = get_local_index(args.named.at("RETURN")),
                        };
                    case builtin_allocator_kind::constexpr_alloc_multiple:
                        if (!args.named.contains("RETURN") || args.positional.size() != 1 || args.size() != 2)
                        {
                            throw compiler_bug("CONSTEXPR_ALLOC_MULTIPLE intrinsic expects a count argument and RETURN slot");
                        }
                        return vmir2::constexpr_alloc_multiple{
                            .storage_type = storage_type,
                            .count = get_local_index(args.positional.at(0)),
                            .result = get_local_index(args.named.at("RETURN")),
                        };
                    case builtin_allocator_kind::constexpr_dealloc:
                        if (args.positional.size() != 1 || args.size() != 1)
                        {
                            throw compiler_bug("CONSTEXPR_DEALLOC intrinsic expects one pointer argument");
                        }
                        return vmir2::constexpr_dealloc{
                            .storage_type = storage_type,
                            .pointer = get_local_index(args.positional.at(0)),
                        };
                    case builtin_allocator_kind::constexpr_dealloc_multiple:
                        if (args.positional.size() != 2 || args.size() != 2)
                        {
                            throw compiler_bug("CONSTEXPR_DEALLOC_MULTIPLE intrinsic expects pointer and count arguments");
                        }
                        return vmir2::constexpr_dealloc_multiple{
                            .storage_type = storage_type,
                            .pointer = get_local_index(args.positional.at(0)),
                            .count = get_local_index(args.positional.at(1)),
                        };
                    }

                    throw compiler_bug("Unhandled constexpr allocator intrinsic kind");
                }
            }

            {
                auto instanciation = func.cast_ptr< instanciation_reference >();
                auto builtin = instanciation == nullptr ? nullptr : instanciation->temploid.templexoid.cast_ptr< builtin_symbol >();
                if (builtin != nullptr && (builtin->name == "IEEE_EQUALS" || builtin->name == "IEEE_NOTEQUALS" || builtin->name == "IEEE_LESS" || builtin->name == "IEEE_GREATER"))
                {
                    if (call.positional.size() != 2 || !call.named.empty() || args.positional.size() != 2 || !args.named.contains("RETURN") || args.size() != 3)
                    {
                        throw compiler_bug(builtin->name + " intrinsic expects two positional arguments and a RETURN slot");
                    }
                    if (call.positional.at(0) != call.positional.at(1) || !typeis< float_type >(call.positional.at(0)))
                    {
                        throw semantic_compilation_error(builtin->name + " requires two floating point arguments of the same type");
                    }

                    auto const a = get_local_index(args.positional.at(0));
                    auto const b = get_local_index(args.positional.at(1));
                    auto const result = get_local_index(args.named.at("RETURN"));
                    if (builtin->name == "IEEE_EQUALS")
                    {
                        return vmir2::float_ieee_eq{.a = a, .b = b, .result = result};
                    }
                    if (builtin->name == "IEEE_NOTEQUALS")
                    {
                        return vmir2::float_ieee_ne{.a = a, .b = b, .result = result};
                    }
                    if (builtin->name == "IEEE_LESS")
                    {
                        return vmir2::float_ieee_lt{.a = a, .b = b, .result = result};
                    }
                    return vmir2::float_ieee_gt{.a = a, .b = b, .result = result};
                }
            }

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

            temploid_reference const* selection = &instanciation->temploid;
            assert(selection);

            instanciation_reference const* member_template_instanciation = nullptr;
            submember const* member = selection->templexoid.cast_ptr< submember >();
            if (member == nullptr)
            {
                member_template_instanciation = selection->templexoid.cast_ptr< instanciation_reference >();
                if (member_template_instanciation != nullptr)
                {
                    member = member_template_instanciation->temploid.templexoid.cast_ptr< submember >();
                }
            }
            if (member == nullptr)
            {
                return std::nullopt;
            }

            if (std::optional< type_symbol > atomic_value_type = atomic_type_argument(*cls); atomic_value_type.has_value())
            {
                if (member->name == "CONSTRUCTOR")
                {
                    if (call.named.contains("OTHER") && args.named.contains("THIS") && args.named.contains("OTHER") && args.size() == 2)
                    {
                        type_symbol const& other_type = call.named.at("OTHER");
                        if (is_ref(other_type) && remove_ref(other_type) == *atomic_value_type)
                        {
                            return vmir2::load_from_ref{
                                .from_reference = get_local_index(args.named.at("OTHER")),
                                .to_value = get_local_index(args.named.at("THIS")),
                                .access_mode = atomic_access_mode::nonatomic,
                            };
                        }
                    }
                }
                else if (member->name == "LOAD")
                {
                    if (member_template_instanciation == nullptr)
                    {
                        throw compiler_bug("atomic LOAD intrinsic expects a member template instantiation");
                    }
                    atomic_access_mode mode = single_atomic_mode_from_instanciation(*member_template_instanciation);
                    if (args.named.contains("THIS") && args.named.contains("RETURN") && args.size() == 2)
                    {
                        return vmir2::load_from_ref{
                            .from_reference = get_local_index(args.named.at("THIS")),
                            .to_value = get_local_index(args.named.at("RETURN")),
                            .access_mode = mode,
                        };
                    }
                }
                else if (member->name == "STORE")
                {
                    if (member_template_instanciation == nullptr)
                    {
                        throw compiler_bug("atomic STORE intrinsic expects a member template instantiation");
                    }
                    atomic_access_mode mode = single_atomic_mode_from_instanciation(*member_template_instanciation);
                    if (args.named.contains("THIS") && args.positional.size() == 1 && args.size() == 2)
                    {
                        return vmir2::store_to_ref{
                            .from_value = get_local_index(args.positional.at(0)),
                            .to_reference = get_local_index(args.named.at("THIS")),
                            .access_mode = mode,
                        };
                    }
                }
                else if (member->name == "COMPARE_EXCHANGE")
                {
                    if (member_template_instanciation == nullptr)
                    {
                        throw compiler_bug("atomic compare_exchange intrinsic expects a member template instantiation");
                    }
                    std::pair< atomic_access_mode, atomic_access_mode > modes = compare_exchange_atomic_modes_from_instanciation(*member_template_instanciation);
                    if (args.named.contains("THIS") && args.named.contains("RETURN") && args.positional.size() == 2 && args.size() == 4)
                    {
                        return vmir2::compare_exchange{
                            .target_reference = get_local_index(args.named.at("THIS")),
                            .expected_reference = get_local_index(args.positional.at(0)),
                            .desired_value = get_local_index(args.positional.at(1)),
                            .result = get_local_index(args.named.at("RETURN")),
                            .success_mode = modes.first,
                            .failure_mode = modes.second,
                        };
                    }
                }
                else if (args.named.contains("THIS") && args.positional.size() == 1)
                {
                    if (member_template_instanciation == nullptr)
                    {
                        throw compiler_bug("atomic RMW intrinsic expects a member template instantiation");
                    }
                    atomic_access_mode mode = single_atomic_mode_from_instanciation(*member_template_instanciation);
                    bool const has_return = args.named.contains("RETURN");
                    std::optional< local_index > old_value = has_return ? std::optional< local_index >{get_local_index(args.named.at("RETURN"))} : std::nullopt;
                    local_index const target = get_local_index(args.named.at("THIS"));
                    local_index const value = get_local_index(args.positional.at(0));

                    if (member->name == "FETCH_ADD" && has_return && args.size() == 3)
                    {
                        return vmir2::mut_int_add{.target = target, .value = value, .access_mode = mode, .old_value = old_value};
                    }
                    if (member->name == "FETCH_SUB" && has_return && args.size() == 3)
                    {
                        return vmir2::mut_int_sub{.target = target, .value = value, .access_mode = mode, .old_value = old_value};
                    }
                    if (member->name == "FETCH_AND" && has_return && args.size() == 3)
                    {
                        return vmir2::mut_bitwise_and{.target = target, .value = value, .access_mode = mode, .old_value = old_value};
                    }
                    if (member->name == "FETCH_OR" && has_return && args.size() == 3)
                    {
                        return vmir2::mut_bitwise_or{.target = target, .value = value, .access_mode = mode, .old_value = old_value};
                    }
                    if (member->name == "FETCH_XOR" && has_return && args.size() == 3)
                    {
                        return vmir2::mut_bitwise_xor{.target = target, .value = value, .access_mode = mode, .old_value = old_value};
                    }
                    if (member->name == "ADD" && !has_return && args.size() == 2)
                    {
                        return vmir2::mut_int_add{.target = target, .value = value, .access_mode = mode};
                    }
                    if (member->name == "SUB" && !has_return && args.size() == 2)
                    {
                        return vmir2::mut_int_sub{.target = target, .value = value, .access_mode = mode};
                    }
                    if (member->name == "AND" && !has_return && args.size() == 2)
                    {
                        return vmir2::mut_bitwise_and{.target = target, .value = value, .access_mode = mode};
                    }
                    if (member->name == "OR" && !has_return && args.size() == 2)
                    {
                        return vmir2::mut_bitwise_or{.target = target, .value = value, .access_mode = mode};
                    }
                    if (member->name == "XOR" && !has_return && args.size() == 2)
                    {
                        return vmir2::mut_bitwise_xor{.target = target, .value = value, .access_mode = mode};
                    }
                }
            }

            if (member->name == "CONSTRUCTOR" && cls->template type_is< constexpr_proxy >())
            {
                if (call.named.contains("THIS") && call.named.contains("OTHER") && args.size() == 2)
                {
                    return vmir2::load_from_ref{.from_reference = get_local_index(args.named.at("OTHER")), .to_value = get_local_index(args.named.at("THIS"))};
                }
            }

            if ((member->name == "OPERATOR++" || member->name == "OPERATOR->") && cls->template type_is< constexpr_proxy >())
            {
                if (call.named.contains("THIS") && args.named.contains("THIS") && args.named.contains("RETURN") && args.size() == 2)
                {
                    return vmir2::copy_reference{
                        .from_index = get_local_index(args.named.at("THIS")),
                        .to_index = get_local_index(args.named.at("RETURN")),
                    };
                }
            }

            if (member->name == "OPERATOR:=" && cls->template type_is< constexpr_proxy >())
            {
                if (call.named.contains("THIS") && call.named.contains("OTHER") && args.named.contains("THIS") && args.named.contains("OTHER") && args.size() == 2)
                {
                    auto const& other_type = call.named.at("OTHER");
                    if (typeis< byte_type >(other_type))
                    {
                        return vmir2::constexpr_output_byte{
                            .proxy = get_local_index(args.named.at("THIS")),
                            .value = get_local_index(args.named.at("OTHER")),
                        };
                    }
                    if (typeis< constexpr_proxy >(other_type))
                    {
                        return vmir2::store_to_ref{
                            .from_value = get_local_index(args.named.at("OTHER")),
                            .to_reference = get_local_index(args.named.at("THIS")),
                        };
                    }
                    throw compiler_bug("constexpr proxy assignment intrinsic requires BYTE or __CONSTEXPR_PROXY input, got: " + quxlang::to_string(other_type));
                }
            }

            if (member->name == "OPERATOR??" || member->name == "OPERATOR?!")
            {
                if ((cls->template type_is< ptrref_type >() && cls->as< ptrref_type >().ptr_class != pointer_class::ref) || cls->template type_is< int_type >())
                {
                    if (args.named.contains("THIS") && args.named.contains("RETURN") && args.size() == 2)
                    {
                        auto this_slot_id = args.named.at("THIS");

                        if (member->name == "OPERATOR??")
                        {
                            vmir2::to_bool tb{};
                            tb.from = get_local_index(this_slot_id);
                            tb.to = get_local_index(args.named.at("RETURN"));
                            return tb;
                        }
                        vmir2::to_bool_not tbn{};
                        tbn.from = get_local_index(this_slot_id);
                        tbn.to = get_local_index(args.named.at("RETURN"));
                        return tbn;
                    }
                }
            }

            if (member->name == "OPERATOR()")
            {
                std::optional< procedure_type > proc;
                if (cls->template type_is< procedure_type >())
                {
                    proc = cls->template get_as< procedure_type >();
                }
                else if (cls->template type_is< ptrref_type >())
                {
                    auto const& ptr = cls->template get_as< ptrref_type >();
                    if (ptr.ptr_class == pointer_class::instance && typeis< procedure_type >(ptr.target))
                    {
                        proc = as< procedure_type >(ptr.target);
                    }
                }

                if (proc.has_value() && args.named.contains("THIS"))
                {
                    vmir2::invoke_indirect inv;
                    inv.what_index = get_local_index(args.named.at("THIS"));
                    for (auto const arg : args.positional)
                    {
                        inv.args.positional.push_back(get_local_index(arg));
                    }
                    for (auto const& [name, _] : proc->signature.params.named)
                    {
                        inv.args.named[name] = get_local_index(args.named.at(name));
                    }
                    if (args.named.contains("RETURN"))
                    {
                        inv.args.named["RETURN"] = get_local_index(args.named.at("RETURN"));
                    }
                    return inv;
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
                else if (cls->template type_is< ptrref_type >())
                {
                    auto const& ptr = cls->as< ptrref_type >();
                    if (ptr.ptr_class == pointer_class::array && call.named.contains("THIS") && call.positional.size() == 1 && args.size() == 3)
                    {
                        auto this_slot_id = args.named.at("THIS");
                        auto index_slot_id = args.positional.at(0);
                        auto return_slot_id = args.named.at("RETURN");

                        if (member->name == "OPERATOR[]")
                        {
                            vmir2::access_pointer acp{};
                            acp.base_index = get_local_index(this_slot_id);
                            acp.index_index = get_local_index(index_slot_id);
                            acp.store_index = get_local_index(return_slot_id);

                            return acp;
                        }

                        vmir2::pointer_arith par{};
                        par.from = get_local_index(this_slot_id);
                        par.multiplier = 1;
                        par.offset = get_local_index(index_slot_id);
                        par.result = get_local_index(return_slot_id);

                        return par;
                    }
                }
            }

            if (member->name == "CONSTRUCTOR")
            {
                std::optional< std::string > ctor_input_name;
                for (std::string const& candidate_name : {"OTHER", "EXPLICIT", "REINTERPRET", "CHECKED", "ASSUME", "PARTIAL", "APPROXIMATE"})
                {
                    if (call.named.contains(candidate_name))
                    {
                        ctor_input_name = candidate_name;
                        break;
                    }
                }

                if (ctor_input_name.has_value())
                {
                    auto const& other = call.named.at(*ctor_input_name);
                    auto other_slot_id = args.named.at(*ctor_input_name);
                    if (cls->template type_is< readonly_constant >())
                    {
                        auto const ro = cls->as< readonly_constant >();
                        // Numeric literal to readonly constant
                        if (other.type_is< numeric_literal_reference >() && ro.kind == constant_kind::numeric)
                        {
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
                            auto const& other_slot = this->state.genvalues.at(other_slot_id);

                            auto const& other_literal = other_slot.template get_as< codegen_literal >();
                            auto const& other_slot_value = other_literal.value;

                            vmir2::load_const_value lcv_result;
                            lcv_result.value = other_slot_value;
                            lcv_result.target = get_local_index(args.named.at("THIS"));
                            return lcv_result;
                        }

                        if (other.type_is< ptrref_type >() && other.as< ptrref_type >().ptr_class == pointer_class::ref && remove_ref(other) == *cls)
                        {
                            vmir2::load_from_ref lfr{};
                            lfr.from_reference = get_local_index(other_slot_id);
                            lfr.to_value = get_local_index(args.named.at("THIS"));
                            return lfr;
                        }
                    }
                    else if ((cls->template type_is< int_type >() || cls->template type_is< byte_type >()) && other.type_is< numeric_literal_reference >())
                    {
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
                    else if (cls->template type_is< float_type >() && other.type_is< numeric_literal_reference >())
                    {
                        auto const& other_slot = this->state.genvalues.at(other_slot_id);

                        assert(other_slot.template type_is< codegen_literal >());

                        auto const& other_literal = other_slot.template get_as< codegen_literal >();
                        auto const& other_slot_value = other_literal.value;

                        vmir2::load_const_float result;
                        for (auto byte : other_slot_value)
                        {
                            result.value.push_back(static_cast< char >(byte));
                        }
                        result.target = get_local_index(args.named.at("THIS"));
                        result.require_exact = *ctor_input_name != "APPROXIMATE";

                        return result;
                    }
                    else if (cls->template type_is< float_type >() && typeis_oneof< int_type, byte_type >(other))
                    {
                        if (*ctor_input_name != "APPROXIMATE")
                        {
                            auto const& other_slot = this->state.genvalues.at(other_slot_id);
                            if (other_slot.template type_is< codegen_literal >())
                            {
                                auto const& other_literal = other_slot.template get_as< codegen_literal >();
                                bytemath::fixed_int_options int_opts{};
                                if (other_literal.type.template type_is< byte_type >())
                                {
                                    int_opts.bits = 8;
                                    int_opts.has_sign = false;
                                }
                                else if (other_literal.type.template type_is< int_type >())
                                {
                                    auto const& int_type_info = other_literal.type.template get_as< int_type >();
                                    int_opts.bits = int_type_info.bits;
                                    int_opts.has_sign = int_type_info.has_sign;
                                }
                                else
                                {
                                    throw compiler_bug("Expected integer literal source for float_from_int generation");
                                }
                                int_opts.overflow_undefined = false;

                                auto const& float_type_info = cls->template get_as< float_type >();
                                bytemath::fixed_float_options float_opts{
                                    .bits = float_type_info.bits,
                                    .exponent_bits = float_type_info.exponent_bits,
                                };

                                auto const converted = bytemath::fixed_float_from_int_le(float_opts, int_opts, other_literal.value, false);
                                if (converted.result_is_undefined)
                                {
                                    throw semantic_compilation_error("integer is outside the finite floating point range");
                                }
                                if (!converted.result_is_exact)
                                {
                                    throw semantic_compilation_error("integer cannot be exactly represented by floating point type; use APPROXIMATE");
                                }
                            }
                        }

                        vmir2::float_from_int result;
                        result.source = get_local_index(other_slot_id);
                        result.result = get_local_index(args.named.at("THIS"));
                        return result;
                    }
                    else if ((cls->template type_is< int_type >() || cls->template type_is< byte_type >()) && typeis_oneof< int_type, byte_type >(other))
                    {
                        vmir2::iconv result;
                        result.from = get_local_index(other_slot_id);
                        result.to = get_local_index(args.named.at("THIS"));

                        if (*ctor_input_name == "CHECKED")
                        {
                            result.convtype = vmir2::conversion_class::checked;
                        }
                        else if (*ctor_input_name == "PARTIAL")
                        {
                            result.convtype = vmir2::conversion_class::partial;
                        }
                        else
                        {
                            result.convtype = vmir2::conversion_class::assume;
                        }

                        return result;
                    }
                    else if (other.type_is< ptrref_type >() && other.as< ptrref_type >().ptr_class == pointer_class::ref && remove_ref(other) == *cls && (!cls->type_is< ptrref_type >() || cls->as< ptrref_type >().ptr_class != pointer_class::ref))
                    {
                        auto this_slot_id = args.named.at("THIS");

                        vmir2::load_from_ref lfr{};
                        lfr.from_reference = get_local_index(other_slot_id);
                        lfr.to_value = get_local_index(this_slot_id);

                        return lfr;
                    }
                    else if (cls->type_is< ptrref_type >() && other.type_is< ptrref_type >() && cls->as< ptrref_type >().ptr_class == pointer_class::ref && other.as< ptrref_type >().ptr_class == pointer_class::ref)
                    {
                        auto this_slot_id = args.named.at("THIS");

                        vmir2::cast_ptrref crf;
                        crf.source_index = get_local_index(other_slot_id);
                        crf.target_index = get_local_index(this_slot_id);
                        return crf;
                    }
                    else if (cls->type_is< ptrref_type >() && other.type_is< ptrref_type >() && cls->as< ptrref_type >().ptr_class != pointer_class::ref && other.as< ptrref_type >().ptr_class != pointer_class::ref)
                    {
                        auto this_slot_id = args.named.at("THIS");

                        vmir2::cast_ptrref crf;
                        crf.source_index = get_local_index(other_slot_id);
                        crf.target_index = get_local_index(this_slot_id);
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
                if (cls->template type_is< int_type >() || cls->template type_is< byte_type >() || cls->template type_is< float_type >() || cls->template type_is< bool_type >() || cls->template type_is< ptrref_type >())
                {
                    if (call.named.contains("OTHER") && call.named.contains("THIS") && call.size() == 2)
                    {
                        auto const& other = call.named.at("OTHER");
                        auto const& this_ = call.named.at("THIS");

                        if ((other == *cls) && is_ref(this_) && remove_ref(this_) == *cls)
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
                if (implement_mut_binary_instruction< vmir2::mut_int_add >(instr, "+=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_int_sub >(instr, "-=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_int_mul >(instr, "*=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_int_div >(instr, "/=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_int_mod >(instr, "%=", *member, call, args))
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
                if (implement_binary_instruction< vmir2::bitwise_implies >(instr, "#^>", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::bitwise_implied >(instr, "#^<", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_and >(instr, "#&&=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_or >(instr, "#||=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_xor >(instr, "#^^=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_nand >(instr, "#&!=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_nor >(instr, "#|!=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_nxor >(instr, "#^!=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_implies >(instr, "#^>=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_implied >(instr, "#^<=", *member, call, args))
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
                if (implement_mut_shift_instruction< vmir2::mut_bitwise_shift_up >(instr, "#++=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_shift_instruction< vmir2::mut_bitwise_shift_down >(instr, "#--=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_shift_instruction< vmir2::mut_bitwise_rotate_up >(instr, "#+%=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_shift_instruction< vmir2::mut_bitwise_rotate_down >(instr, "#-%=", *member, call, args))
                {
                    return instr;
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
            else if (cls->template type_is< float_type >())
            {
                std::optional< vmir2::vm_instruction > instr;
                if (implement_binary_instruction< vmir2::float_add >(instr, "+", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::float_sub >(instr, "-", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::float_mul >(instr, "*", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::float_div >(instr, "/", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_float_add >(instr, "+=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_float_sub >(instr, "-=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_float_mul >(instr, "*=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_float_div >(instr, "/=", *member, call, args))
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
            else if (cls->template type_is< byte_type >())
            {
                std::optional< vmir2::vm_instruction > instr;
                if (implement_mut_binary_instruction< vmir2::mut_int_add >(instr, "+=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_int_sub >(instr, "-=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_int_mul >(instr, "*=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_int_div >(instr, "/=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_int_mod >(instr, "%=", *member, call, args))
                {
                    return instr;
                }
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
                if (implement_binary_instruction< vmir2::bitwise_implies >(instr, "#^>", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_binary_instruction< vmir2::bitwise_implied >(instr, "#^<", true, *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_and >(instr, "#&&=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_or >(instr, "#||=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_xor >(instr, "#^^=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_nand >(instr, "#&!=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_nor >(instr, "#|!=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_nxor >(instr, "#^!=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_implies >(instr, "#^>=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_binary_instruction< vmir2::mut_bitwise_implied >(instr, "#^<=", *member, call, args))
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
                if (implement_mut_shift_instruction< vmir2::mut_bitwise_shift_up >(instr, "#++=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_shift_instruction< vmir2::mut_bitwise_shift_down >(instr, "#--=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_shift_instruction< vmir2::mut_bitwise_rotate_up >(instr, "#+%=", *member, call, args))
                {
                    return instr;
                }
                if (implement_mut_shift_instruction< vmir2::mut_bitwise_rotate_down >(instr, "#-%=", *member, call, args))
                {
                    return instr;
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
                else if (implement_binary_instruction< vmir2::cmp_lt >(instr, "<", true, *member, call, args))
                {
                    return instr;
                }
                else if (implement_binary_instruction< vmir2::cmp_lt >(instr, ">", true, *member, call, args, true))
                {
                    return instr;
                }
                else if (implement_binary_instruction< vmir2::cmp_ge >(instr, "<=", true, *member, call, args, true))
                {
                    return instr;
                }
                else if (implement_binary_instruction< vmir2::cmp_ge >(instr, ">=", true, *member, call, args))
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

        auto co_gen_invoke(block_index& bidx, instanciation_reference what, codegen_invocation_args args) -> co_type< void >
        {
            auto builtin_kind = co_await rpnx::querygraph::request< function_builtin_query >(what.temploid);
            if (builtin_kind != builtin_function_kind::not_builtin)
            {
                co_return co_await co_gen_invoke_builtin(bidx, what, args);
            }

            if (typeis< submember >(what.temploid.templexoid))
            {
                submember const& member = as< submember >(what.temploid.templexoid);
                if (co_await rpnx::querygraph::request< symbol_type_query >(member.of) == symbol_kind::interface_)
                {
                    if (!args.named.contains("THIS"))
                    {
                        throw compiler_bug("Interface invocation is missing THIS");
                    }

                    value_index interface_value = args.named.at("THIS");
                    type_symbol interface_value_type = this->current_type(bidx, interface_value);
                    if (is_ref(interface_value_type))
                    {
                        type_symbol referenced_type = remove_ref(interface_value_type);
                        if (referenced_type != member.of)
                        {
                            throw semantic_compilation_error(
                                "Interface invocation expected THIS to reference " + to_string(member.of) + ", got " + to_string(interface_value_type));
                        }
                        interface_value = load_reference_value(bidx, interface_value, referenced_type);
                    }
                    else if (interface_value_type != member.of)
                    {
                        throw semantic_compilation_error(
                            "Interface invocation expected THIS to be " + to_string(member.of) + ", got " + to_string(interface_value_type));
                    }

                    interface_slot_key key = co_await interface_slot_key_from_functanoid(what);
                    codegen_invocation_args call_args = args;
                    call_args.named.erase("THIS");

                    vmir2::interface_invoke inv;
                    inv.interface_value = get_local_index(interface_value);
                    inv.slot = key;
                    inv.args = get_invocation_args(call_args);
                    if (co_await interface_slot_has_default_body(member.of, key))
                    {
                        inv.default_function = what;
                    }
                    this->emit(bidx, inv);
                    co_return;
                }
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
            instatype concrete_params = co_await rpnx::querygraph::request< instanciation_concrete_params_query >(what);

            vmir2::invoke ivk;
            ivk.what = what;
            ivk.args = get_invocation_args(concrete_params, args);

            this->emit(bidx, ivk);
            co_return;
        }

        auto co_generate(block_index& bidx, expression_symbol_reference expr) -> co_type< value_index >
        {
            std::string sym = quxlang::to_string(expr.symbol);
            auto value_opt = (co_await this->co_lookup_symbol(bidx, expr.symbol));

            if (!value_opt.has_value())
            {
                throw semantic_compilation_error("Expected symbol " + quxlang::to_string(expr.symbol) + " to be defined.");
            }

            co_return value_opt.value();
        }

        auto co_generate(block_index& bidx, expression_forward expr) -> co_type< value_index >
        {
            if (!expr.symbol.template type_is< freebound_identifier >())
            {
                throw semantic_compilation_error("FORWARD requires a symbol");
            }

            auto const& name = expr.symbol.template get_as< freebound_identifier >().name;
            auto value = this->local_value_direct_lookup(bidx, name);
            if (!value.has_value())
            {
                throw semantic_compilation_error("FORWARD requires a visible local or parameter symbol");
            }

            auto declared_type = this->declared_type_of_local_value(*value);
            if (!is_ref(declared_type))
            {
                throw semantic_compilation_error("FORWARD currently requires a reference-typed symbol");
            }

            auto current = this->current_type(bidx, *value);
            if (!is_ref(current))
            {
                throw semantic_compilation_error("FORWARD target is not currently a reference");
            }

            co_return this->copy_refernece_internal(bidx, *value);
        }

        auto co_generate(block_index& bidx, expression_snapshot expr) -> co_type< value_index >
        {
            auto symbol = this->find_visible_static_binding(expr.name);
            if (!symbol.has_value())
            {
                throw semantic_compilation_error("SNAPSHOT requires a visible function-local static: " + expr.name);
            }

            std::map< static_local_ref, static_snapshot_ref > remapped;
            auto snapshot_symbol = this->create_ordinary_snapshot_for_binding(*symbol, remapped, true);
            auto const& binding = this->state.statics.at(*symbol);
            co_return this->create_antestatal_reference(bidx, type_symbol(snapshot_symbol), binding.type, false);
        }

        /// Generates a numeric literal for a positional pack's compile-time size.
        auto co_generate(block_index& bidx, expression_pack_size expr) -> co_type< value_index >
        {
            (void)bidx;
            auto const pack_it = this->state.packs.find(expr.pack_name);
            if (pack_it != this->state.packs.end())
            {
                co_return this->create_numeric_literal(std::to_string(pack_it->second.values.size()));
            }

            if (this->ctx.template type_is< instanciation_reference >())
            {
                auto pack_info = co_await rpnx::querygraph::request< function_pack_info_query >(this->ctx.template get_as< instanciation_reference >());
                auto const info_it = pack_info.packs.find(expr.pack_name);
                if (info_it != pack_info.packs.end())
                {
                    co_return this->create_numeric_literal(std::to_string(info_it->second.size));
                }
            }

            {
                throw semantic_compilation_error("Unknown positional pack '" + expr.pack_name + "'");
            }
        }

        /// Generates a reference to one concrete parameter captured by a positional pack.
        auto co_generate(block_index& bidx, expression_pack_arg expr) -> co_type< value_index >
        {
            auto const pack_it = this->state.packs.find(expr.pack_name);
            if (pack_it == this->state.packs.end())
            {
                throw semantic_compilation_error("Unknown positional pack '" + expr.pack_name + "'");
            }

            std::uint64_t const index = co_await this->co_constexpr_u64(bidx, expr.index);
            if (index >= pack_it->second.values.size())
            {
                throw semantic_compilation_error("PACK_ARG index is out of range for positional pack '" + expr.pack_name + "'");
            }

            co_return this->materialize_lookup_reference(bidx, pack_it->second.values.at(static_cast< std::vector< value_index >::size_type >(index)));
        }

        auto co_generate(block_index& bidx, expression_sizeof szof) -> co_type< value_index >
        {
            auto type_opt = co_await this->co_lookup_symbol(bidx, szof.of_type);
            if (!type_opt.has_value())
            {
                throw semantic_compilation_error("Expected type " + quxlang::to_string(szof.of_type) + " to be defined.");
            }

            auto type_val = type_opt.value();

            auto const& genvalue = this->state.genvalues.at(type_val);

            if (genvalue.template type_is< codegen_literal >())
            {
                throw semantic_compilation_error("Expected SIZEOF(...) to refer to a class type, got a literal genvalue instead (hint: cast to a concrete type like I32, NUMERIC_CONSTANT, STRING_CONSTANT, or similar).");
            }

            if (genvalue.template type_is< codegen_local >())
            {
                throw semantic_compilation_error("Expected SIZEOF(...) to refer to a class type, got an object or reference instead.");
            }

            if (!genvalue.template type_is< codegen_binding >())
            {
                throw semantic_compilation_error("Expected SIZEOF(...) to refer to a class type, got something else instead.");
            }
            auto const& binding = genvalue.template get_as< codegen_binding >();
            if (binding.bound_value != value_index(0))
            {
                throw semantic_compilation_error("Expected SIZEOF(...) to refer to a class type, got an attached symbol (member function?) instead. (hint: cast member function attachments to a concrete type first)");
            }

            auto const& attached_type = binding.attached_symbol;
            assert(!type_is_contextual(attached_type));

            symbol_kind kind = co_await rpnx::querygraph::request< symbol_type_query >(attached_type);
            if (kind != symbol_kind::class_)
            {
                throw semantic_compilation_error("Expected SIZEOF(...) to refer to a class type, got a non-class type instead.");
            }

            type_placement_info placement_info = co_await rpnx::querygraph::request< type_placement_info_query >(attached_type);

            auto lit = this->create_numeric_literal(std::to_string(placement_info.size));

            co_return lit;
        }

        auto co_generate(block_index& bidx, expression_bits szof) -> co_type< value_index >
        {
            auto type_opt = co_await this->co_lookup_symbol(bidx, szof.of_type);
            if (!type_opt.has_value())
            {
                throw semantic_compilation_error("Expected type " + quxlang::to_string(szof.of_type) + " to be defined.");
            }

            auto type_val = type_opt.value();

            auto const& genvalue = this->state.genvalues.at(type_val);

            if (genvalue.template type_is< codegen_literal >())
            {
                throw semantic_compilation_error("Expected BITS(...) to refer to a integer type, got a literal genvalue instead (hint: cast to a concrete type like I32, NUMERIC_CONSTANT, STRING_CONSTANT, or similar).");
            }

            if (genvalue.template type_is< codegen_local >())
            {
                throw semantic_compilation_error("Expected BITS(...) to refer to a integer type, got an object or reference instead.");
            }

            if (!genvalue.template type_is< codegen_binding >())
            {
                throw semantic_compilation_error("Expected BITS(...) to refer to a integer type, got something else instead.");
            }
            auto const& binding = genvalue.template get_as< codegen_binding >();
            if (binding.bound_value != value_index(0))
            {
                throw semantic_compilation_error("Expected BITS(...) to refer to a integer type, got an attached symbol (member function?) instead. (hint: cast member function attachments to a concrete type first)");
            }

            type_symbol const& attached_type = binding.attached_symbol;
            assert(!type_is_contextual(attached_type));

            symbol_kind kind = co_await rpnx::querygraph::request< symbol_type_query >(attached_type);
            if (kind != symbol_kind::class_)
            {
                throw semantic_compilation_error("Expected BITS(...) to refer to an integer type, got a non-class type instead.");
            }

            if (attached_type.template type_is< byte_type >())
            {
                co_return this->create_numeric_literal("8");
            }

            if (!attached_type.template type_is< int_type >())
            {
                throw semantic_compilation_error("Expected BITS(...) to refer to an integer type, got a non-integer class type instead.");
            }

            int_type const& inttype = attached_type.template as< int_type >();
            auto lit = this->create_numeric_literal(std::to_string(inttype.bits));

            co_return lit;
        }

        auto co_generate(block_index& bidx, expression_is_signed szof) -> co_type< value_index >
        {
            auto type_opt = co_await this->co_lookup_symbol(bidx, szof.of_type);
            if (!type_opt.has_value())
            {
                throw semantic_compilation_error("Expected type " + quxlang::to_string(szof.of_type) + " to be defined.");
            }

            auto type_val = type_opt.value();

            auto const& genvalue = this->state.genvalues.at(type_val);

            if (genvalue.template type_is< codegen_literal >())
            {
                throw semantic_compilation_error("Expected BITS(...) to refer to a integer type, got a literal genvalue instead (hint: cast to a concrete type like I32, NUMERIC_CONSTANT, STRING_CONSTANT, or similar).");
            }

            if (genvalue.template type_is< codegen_local >())
            {
                throw semantic_compilation_error("Expected BITS(...) to refer to a integer type, got an object or reference instead.");
            }

            if (!genvalue.template type_is< codegen_binding >())
            {
                throw semantic_compilation_error("Expected BITS(...) to refer to a integer type, got something else instead.");
            }
            auto const& binding = genvalue.template get_as< codegen_binding >();
            if (binding.bound_value != value_index(0))
            {
                throw semantic_compilation_error("Expected BITS(...) to refer to a integer type, got an attached symbol (member function?) instead. (hint: cast member function attachments to a concrete type first)");
            }

            type_symbol const& attached_type = binding.attached_symbol;
            assert(!type_is_contextual(attached_type));

            symbol_kind kind = co_await rpnx::querygraph::request< symbol_type_query >(attached_type);
            if (kind != symbol_kind::class_)
            {
                throw semantic_compilation_error("Expected BITS(...) to refer to an integer type, got a non-class type instead.");
            }

            if (attached_type.template type_is< byte_type >())
            {
                co_return this->create_bool_value(bidx, false);
            }

            if (!attached_type.template type_is< int_type >())
            {
                throw semantic_compilation_error("Expected BITS(...) to refer to an integer type, got a non-integer class type instead.");
            }

            int_type const& inttype = attached_type.template as< int_type >();
            co_return this->create_bool_value(bidx, inttype.has_sign);
        }

        auto co_generate(block_index& bidx, expression_is_integral szof) -> co_type< value_index >
        {
            auto type_opt = co_await this->co_lookup_symbol(bidx, szof.of_type);
            if (!type_opt.has_value())
            {
                throw semantic_compilation_error("Expected type " + quxlang::to_string(szof.of_type) + " to be defined.");
            }

            auto type_val = type_opt.value();

            auto const& genvalue = this->state.genvalues.at(type_val);

            if (genvalue.template type_is< codegen_literal >())
            {
                co_return this->create_bool_value(bidx, false);
            }

            if (genvalue.template type_is< codegen_local >())
            {
                throw semantic_compilation_error("Expected IS_INTEGRAL(...) to refer to a type, got an object or reference instead.");
            }

            if (!genvalue.template type_is< codegen_binding >())
            {
                throw semantic_compilation_error("Expected IS_INTEGRAL(...) to refer to a type, got something else instead.");
            }
            auto const& binding = genvalue.template get_as< codegen_binding >();
            if (binding.bound_value != value_index(0))
            {
                throw semantic_compilation_error("Expected IS_INTEGRAL(...) to refer to a type, got an attached symbol (member function?) instead. (hint: cast member function attachments to a concrete type first)");
            }

            type_symbol const& attached_type = binding.attached_symbol;
            assert(!type_is_contextual(attached_type));

            symbol_kind kind = co_await rpnx::querygraph::request< symbol_type_query >(attached_type);
            if (kind != symbol_kind::class_)
            {
                co_return this->create_bool_value(bidx, false);
            }

            if (!typeis< int_type >(attached_type))
            {
                co_return this->create_bool_value(bidx, false);
            }

            co_return this->create_bool_value(bidx, true);
        }

        auto co_generate(block_index& bidx, expression_same_types expr) -> co_type< value_index >
        {
            auto resolve_type_expr = [&](type_symbol const& sym) -> co_type< type_symbol >
            {
                if (sym.template type_is< decltype_type_ref >() || sym.template type_is< typeof_type_ref >())
                {
                    co_return co_await this->co_resolve_type_symbol(bidx, sym);
                }

                auto type_opt = co_await this->co_lookup_symbol(bidx, sym);
                if (!type_opt.has_value())
                {
                    throw semantic_compilation_error("Expected type " + quxlang::to_string(sym) + " to be defined.");
                }

                auto const& genvalue = this->state.genvalues.at(*type_opt);

                if (genvalue.template type_is< codegen_literal >())
                {
                    throw semantic_compilation_error("Expected SAME_TYPES(...) to refer to a type, got a literal instead.");
                }

                if (genvalue.template type_is< codegen_local >())
                {
                    throw semantic_compilation_error("Expected SAME_TYPES(...) to refer to a type, got an object or reference instead.");
                }

                if (!genvalue.template type_is< codegen_binding >())
                {
                    throw semantic_compilation_error("Expected SAME_TYPES(...) to refer to a type, got something else instead.");
                }

                auto const& binding = genvalue.template get_as< codegen_binding >();
                if (binding.bound_value != value_index(0))
                {
                    throw semantic_compilation_error("Expected SAME_TYPES(...) to refer to a type, got an attached symbol (member function?) instead. (hint: cast member function attachments to a concrete type first)");
                }

                co_return binding.attached_symbol;
            };

            auto lhs_type = co_await resolve_type_expr(expr.lhs_type);
            auto rhs_type = co_await resolve_type_expr(expr.rhs_type);

            co_return this->create_bool_value(bidx, lhs_type == rhs_type);
        }

        auto co_generate(block_index& bidx, expression_this_reference expr) -> co_type< value_index >
        {
            throw rpnx::unimplemented();
            co_return value_index(0);
        }

        auto co_generate(block_index& bidx, expression_target target) -> co_type< value_index >
        {
            throw rpnx::unimplemented();
        }

        auto co_generate(block_index& bidx, expression_leftarrow expr) -> co_type< value_index >
        {
            auto value = co_await co_generate_expr(bidx, expr.lhs);

            auto type = this->current_type(bidx, value);
            if (typeis< attached_type_reference >(type))
            {
                auto const& attached = as< attached_type_reference >(type);
                auto kind = co_await rpnx::querygraph::request< symbol_type_query >(attached.attached_symbol);
                if (kind == symbol_kind::funtanoid)
                {
                    if (!typeis< void_type >(attached.carrying_type))
                    {
                        throw semantic_compilation_error("Bound method procedure pointers are not yet supported");
                    }
                    co_return co_await co_gen_get_procedure_ptr(bidx, attached.attached_symbol, "DEFAULT");
                }
                if (kind == symbol_kind::functum)
                {
                    if (!typeis< void_type >(attached.carrying_type))
                    {
                        throw semantic_compilation_error("Bound method procedure pointers are not yet supported");
                    }

                    auto overloads = co_await rpnx::querygraph::request< functum_overloads_query >(attached.attached_symbol);
                    if (overloads.size() != 1)
                    {
                        throw semantic_compilation_error("Cannot take address of overloaded functum " + to_string(attached.attached_symbol));
                    }

                    temploid_reference selected_function{
                        .templexoid = attached.attached_symbol,
                        .overload_id = std::nullopt,
                    };
                    auto selected_overload = co_await rpnx::querygraph::request< temploid_formal_ensig_query >(selected_function);
                    if (!selected_overload.has_value())
                    {
                        throw compiler_bug("Unique functum selection did not resolve to a formal ensig");
                    }

                    bool has_template_params = false;
                    for (auto const& arg : selected_overload->interface.positional)
                    {
                        if (arg.is_pack)
                        {
                            throw semantic_compilation_error("Cannot take address of uninstantiated variadic functum " + to_string(attached.attached_symbol));
                        }
                        if (is_template(arg.type))
                        {
                            has_template_params = true;
                            break;
                        }
                    }
                    for (auto const& [_, arg] : selected_overload->interface.named)
                    {
                        if (arg.is_pack)
                        {
                            throw semantic_compilation_error("Cannot take address of uninstantiated variadic functum " + to_string(attached.attached_symbol));
                        }
                        if (is_template(arg.type))
                        {
                            has_template_params = true;
                            break;
                        }
                    }
                    if (has_template_params)
                    {
                        throw semantic_compilation_error("Cannot take address of templated functum " + to_string(attached.attached_symbol));
                    }

                    instanciation_reference selected_inst;
                    selected_inst.temploid = selected_function;
                    for (auto const& arg : selected_overload->interface.positional)
                    {
                        if (arg.is_pack)
                        {
                            throw semantic_compilation_error("Cannot take address of uninstantiated variadic functum " + to_string(attached.attached_symbol));
                        }
                        selected_inst.params.positional.push_back(make_type_instantiation(arg.type));
                    }
                    for (auto const& [name, arg] : selected_overload->interface.named)
                    {
                        selected_inst.params.named[name] = make_type_instantiation(arg.type);
                    }
                    co_return co_await co_gen_get_procedure_ptr(bidx, selected_inst, "DEFAULT");
                }

                throw semantic_compilation_error("Cannot take address of non-object binding " + to_string(attached.attached_symbol));
            }

            vmir2::make_pointer_to make_pointer;
            make_pointer.of_index = get_local_index(value);

            auto non_ref_type = remove_ref(type);
            qualifier pointer_qual = qualifier::mut;
            if (typeis< ptrref_type >(type) && as< ptrref_type >(type).ptr_class == pointer_class::ref)
            {
                pointer_qual = as< ptrref_type >(type).qual;
            }

            auto pointer_storage = create_local_value(ptrref_type{.target = non_ref_type, .ptr_class = pointer_class::instance, .qual = pointer_qual});

            make_pointer.pointer_index = get_local_index(pointer_storage);

            this->emit(bidx, make_pointer);

            co_return pointer_storage;
        }

        auto co_generate(block_index& bidx, expression_value_keyword const& kw) -> co_type< value_index >
        {
            if (kw.keyword == "TRUE")
            {
                co_return this->create_bool_value(bidx, true);
            }
            if (kw.keyword == "FALSE")
            {
                co_return this->create_bool_value(bidx, false);
            }
            output_info arch = machine_info;

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

            if (kw.keyword == "THIS" || kw.keyword == "OTHER" || kw.keyword == "EXPLICIT" || kw.keyword == "ASSUME")
            {
                auto result = co_await this->co_lookup_symbol(bidx, freebound_identifier{.name = kw.keyword});
                if (!result.has_value())
                {
                    throw quxlang::semantic_compilation_error("Expected symbol " + kw.keyword + " to be defined.");
                }
                co_return result.value();
            }

            throw rpnx::unimplemented();
        }

        auto co_generate(block_index& bidx, expression_static_choose const& sc) -> co_type< value_index >
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
        auto co_generate(block_index& bidx, expression_choose const& ch) -> co_type< value_index >
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

        auto co_converge_values(block_index& output_block, block_index& bidx1, value_index val1, block_index& bidx2, value_index val2) -> co_type< value_index >
        {
            throw rpnx::unimplemented();
        }

        auto co_constexpr_bool(block_index&, expression const& expr) -> co_type< bool >
        {
            if (!this->state.statics.empty())
            {
                auto eval_result = co_await this->co_eval_static_expression(expr, type_symbol(bool_type{}), static_eval_access::readonly_view);
                co_return static_eval_result_as_bool(eval_result);
            }

            auto ce_input = constexpr_input{.context = ctx, .expr = expr};
            for (auto const& [name, def] : this->state.scoped_definitions)
            {
                if (def.template type_is< scoped_typedef >())
                {
                    ce_input.scoped_definitions[name] = def.template get_as< scoped_typedef >().type;
                    continue;
                }
                if (def.template type_is< scoped_static >())
                {
                    ce_input.scoped_static_symbols[name] = def.template get_as< scoped_static >().symbol;
                    continue;
                }
                throw rpnx::unimplemented();
            }
            auto ce_result = co_await rpnx::querygraph::request< constexpr_bool_query >(ce_input);
            co_return ce_result;
        }

        /// Evaluates an expression as a constexpr U64 in the current instantiated function context.
        auto co_constexpr_u64(block_index&, expression const& expr) -> co_type< std::uint64_t >
        {
            if (!this->state.statics.empty())
            {
                auto eval_result = co_await this->co_eval_static_expression(expr, type_symbol(int_type{.bits = 64, .has_sign = false}), static_eval_access::readonly_view);
                auto result_it = eval_result.values.find(constexpr_primary_result_id);
                if (result_it == eval_result.values.end())
                {
                    throw compiler_bug("static u64 evaluation did not produce a primary result");
                }
                auto const& value = constexpr_value_as_antestatal(result_it->second);
                if (!typeis< antestatal_primitive >(value))
                {
                    throw compiler_bug("static u64 evaluation did not produce a primitive result");
                }
                auto [intval, ok] = bytemath::le_to_u< std::uint64_t >(as< antestatal_primitive >(value).value);
                if (!ok)
                {
                    throw compiler_bug("static u64 evaluation produced invalid integer bytes");
                }
                co_return intval;
            }

            auto ce_input = constexpr_input{.context = ctx, .expr = expr};
            for (auto const& [name, def] : this->state.scoped_definitions)
            {
                if (def.template type_is< scoped_typedef >())
                {
                    ce_input.scoped_definitions[name] = def.template get_as< scoped_typedef >().type;
                    continue;
                }
                if (def.template type_is< scoped_static >())
                {
                    ce_input.scoped_static_symbols[name] = def.template get_as< scoped_static >().symbol;
                    continue;
                }
                throw rpnx::unimplemented();
            }
            co_return co_await rpnx::querygraph::request< constexpr_u64_query >(ce_input);
        }

        /// Converts result ID 0 from a static evaluation into a native bool.
        auto static_eval_result_as_bool(constexpr_result_v3 const& result) const -> bool
        {
            auto result_it = result.values.find(constexpr_primary_result_id);
            if (result_it == result.values.end())
            {
                throw compiler_bug("static bool evaluation did not produce a primary result");
            }
            auto const& value = constexpr_value_as_antestatal(result_it->second);
            if (!typeis< antestatal_primitive >(value))
            {
                throw compiler_bug("static bool evaluation did not produce a primitive result");
            }
            auto const& data = as< antestatal_primitive >(value).value;
            if (data == std::vector{std::byte{0}})
            {
                return false;
            }
            if (data == std::vector{std::byte{1}})
            {
                return true;
            }
            throw compiler_bug("static bool evaluation produced invalid bool bytes");
        }

        /// Builds a constexpr query input using the currently visible static bindings.
        auto build_static_eval_input(expression expr, std::optional< type_symbol > expected_result_type, static_eval_access access) -> constexpr_input_v3
        {
            constexpr_input_v3 input;
            input.expr = std::move(expr);
            input.context = this->ctx;
            input.expected_result_type = std::move(expected_result_type);
            input.scoped_definitions = this->state.scoped_definitions;

            for (auto const& [symbol, binding] : this->state.statics)
            {
                auto mutation_result_id = access == static_eval_access::mutable_view ? binding.mutation_result_id : std::nullopt;
                input.statics[symbol] = constexpr_static{
                    .type = binding.type,
                    .value = binding.value,
                    .mutation_result_id = mutation_result_id,
                };
            }
            for (auto const& scope : this->state.static_scopes)
            {
                for (auto const& [name, symbol] : scope.bindings)
                {
                    input.scoped_definitions[name] = scoped_static{.symbol = symbol};
                }
            }
            return input;
        }

        /// Applies returned nonzero result IDs to mutable function-local static bindings.
        auto apply_static_eval_mutations(std::map< std::uint64_t, constexpr_value > const& result_values) -> void
        {
            for (auto& [_, binding] : this->state.statics)
            {
                if (!binding.mutation_result_id.has_value())
                {
                    continue;
                }
                if (auto result_it = result_values.find(*binding.mutation_result_id); result_it != result_values.end())
                {
                    binding.value = result_it->second;
                }
            }
        }

        /// Evaluates an expression immediately with the selected static mutability policy.
        auto co_eval_static_expression(expression expr, std::optional< type_symbol > expected_result_type, static_eval_access access) -> co_type< constexpr_result_v3 >
        {
            bool const require_primary_result = expected_result_type.has_value();
            auto input = build_static_eval_input(std::move(expr), std::move(expected_result_type), access);
            auto result = co_await rpnx::querygraph::request< constexpr_eval_v3_query >(input);
            if (require_primary_result && !result.values.contains(constexpr_primary_result_id))
            {
                throw compiler_bug("static evaluation did not produce a primary result");
            }
            if (access == static_eval_access::mutable_view)
            {
                apply_static_eval_mutations(result.values);
            }
            co_return result;
        }

        auto lambda_reference_capture_type(block_index bidx, value_index value) -> type_symbol
        {
            type_symbol value_type = this->current_type(bidx, value);
            if (!is_ref(value_type))
            {
                return make_mref(value_type);
            }
            ptrref_type ref_type = as< ptrref_type >(value_type);
            if (ref_type.qual == qualifier::write)
            {
                return make_mref(ref_type.target);
            }
            return value_type;
        }

        auto lambda_value_capture_type(block_index bidx, value_index value) -> type_symbol
        {
            return remove_ref(this->current_type(bidx, value));
        }

        auto build_lambda_possible_captures(block_index bidx) -> std::map< std::string, lambda_possible_capture >
        {
            std::map< std::string, lambda_possible_capture > result;
            auto add_lookup = [&](std::string const& name, value_index value)
            {
                if (name == "RETURN" || name == "THIS")
                {
                    return;
                }
                result[name] = lambda_possible_capture{
                    .reference_field_type = this->lambda_reference_capture_type(bidx, value),
                    .value_field_type = this->lambda_value_capture_type(bidx, value),
                };
            };

            for (auto const& [name, value] : this->state.top_level_lookups)
            {
                add_lookup(name, value);
            }
            for (auto const& [name, value] : this->state.top_level_lookups_weak)
            {
                if (!result.contains(name))
                {
                    add_lookup(name, value);
                }
            }
            for (auto const& [name, value] : this->state.blocks.at(bidx).lookup_values)
            {
                add_lookup(name, value);
            }
            return result;
        }

        auto lambda_static_context_from_current() -> lambda_dry_static_context
        {
            return lambda_dry_static_context{
                .scoped_definitions = this->state.scoped_definitions,
                .statics = this->state.statics,
                .static_scopes = this->state.static_scopes,
            };
        }

        auto lambda_environment_from_analysis(lambda_capture_analysis_state const& analysis) -> lambda_environment
        {
            lambda_environment env;
            env.capture_indices = analysis.capture_indices;
            env.scoped_definitions = analysis.static_context.scoped_definitions;
            for (auto const& scope : analysis.static_context.static_scopes)
            {
                for (auto const& [name, symbol] : scope.bindings)
                {
                    env.scoped_definitions[name] = scoped_static{.symbol = symbol};
                }
            }
            for (auto const& [symbol, binding] : analysis.static_context.statics)
            {
                env.statics[symbol] = constexpr_static{
                    .type = binding.type,
                    .value = binding.value,
                    .mutation_result_id = std::nullopt,
                };
            }
            return env;
        }

        auto add_lambda_capture(lambda_capture_analysis_state& analysis, std::string const& name) -> void
        {
            if (analysis.local_types.contains(name))
            {
                return;
            }
            auto possible_it = analysis.possible_captures.find(name);
            if (possible_it == analysis.possible_captures.end())
            {
                return;
            }
            if (analysis.has_explicit_capture_list && !analysis.explicit_captures.contains(name))
            {
                throw semantic_compilation_error("Lambda body references uncaptured outer local: " + name);
            }
            if (analysis.capture_indices.contains(name))
            {
                return;
            }

            lambda_capture_mode mode = lambda_capture_mode::reference;
            if (analysis.has_explicit_capture_list)
            {
                mode = analysis.explicit_captures.at(name);
            }
            type_symbol field_type = mode == lambda_capture_mode::value ? possible_it->second.value_field_type : possible_it->second.reference_field_type;
            std::size_t index = analysis.captures.size();
            analysis.capture_indices[name] = index;
            analysis.captures.push_back(lambda_capture_selection{
                .name = name,
                .mode = mode,
                .field_type = std::move(field_type),
            });
        }

        auto add_lambda_local_capture_source(lambda_capture_analysis_state& analysis, std::string const& name, type_symbol const& type) -> void
        {
            if (is_ref(type))
            {
                analysis.possible_captures[name] = lambda_possible_capture{
                    .reference_field_type = type,
                    .value_field_type = remove_ref(type),
                };
                return;
            }
            analysis.possible_captures[name] = lambda_possible_capture{
                .reference_field_type = make_mref(type),
                .value_field_type = type,
            };
        }

        auto co_eval_lambda_dry_static_expression(lambda_capture_analysis_state& analysis, expression expr, std::optional< type_symbol > expected_result_type) -> co_type< constexpr_result_v3 >
        {
            auto saved_scoped_definitions = std::move(this->state.scoped_definitions);
            auto saved_statics = std::move(this->state.statics);
            auto saved_static_scopes = std::move(this->state.static_scopes);

            this->state.scoped_definitions = analysis.static_context.scoped_definitions;
            this->state.statics = analysis.static_context.statics;
            this->state.static_scopes = analysis.static_context.static_scopes;

            auto result = co_await this->co_eval_static_expression(std::move(expr), std::move(expected_result_type), static_eval_access::mutable_view);

            analysis.static_context.scoped_definitions = std::move(this->state.scoped_definitions);
            analysis.static_context.statics = std::move(this->state.statics);
            analysis.static_context.static_scopes = std::move(this->state.static_scopes);

            this->state.scoped_definitions = std::move(saved_scoped_definitions);
            this->state.statics = std::move(saved_statics);
            this->state.static_scopes = std::move(saved_static_scopes);
            co_return result;
        }

        auto co_analyze_lambda_expression(lambda_capture_analysis_state& analysis, expression const& expr) -> co_type< void >
        {
            co_await rpnx::apply_visitor< co_type< void > >(
                expr,
                [&](auto const& value) -> co_type< void >
                {
                    using value_type = std::decay_t< decltype(value) >;
                    if constexpr (std::is_same_v< value_type, expression_symbol_reference >)
                    {
                        if (typeis< freebound_identifier >(value.symbol))
                        {
                            this->add_lambda_capture(analysis, as< freebound_identifier >(value.symbol).name);
                        }
                    }
                    else if constexpr (std::is_same_v< value_type, expression_binary >)
                    {
                        co_await this->co_analyze_lambda_expression(analysis, value.lhs);
                        co_await this->co_analyze_lambda_expression(analysis, value.rhs);
                    }
                    else if constexpr (std::is_same_v< value_type, expression_unary_prefix >)
                    {
                        co_await this->co_analyze_lambda_expression(analysis, value.rhs);
                    }
                    else if constexpr (std::is_same_v< value_type, expression_unary_postfix > || std::is_same_v< value_type, expression_dotreference > ||
                                       std::is_same_v< value_type, expression_rightarrow > || std::is_same_v< value_type, expression_leftarrow >)
                    {
                        co_await this->co_analyze_lambda_expression(analysis, value.lhs);
                    }
                    else if constexpr (std::is_same_v< value_type, expression_multibind >)
                    {
                        co_await this->co_analyze_lambda_expression(analysis, value.lhs);
                        for (auto const& item : value.bracketed)
                        {
                            co_await this->co_analyze_lambda_expression(analysis, item);
                        }
                    }
                    else if constexpr (std::is_same_v< value_type, expression_call >)
                    {
                        co_await this->co_analyze_lambda_expression(analysis, value.callee);
                        for (auto const& arg : value.args)
                        {
                            co_await this->co_analyze_lambda_expression(analysis, arg.value);
                        }
                    }
                    else if constexpr (std::is_same_v< value_type, expression_typecast >)
                    {
                        co_await this->co_analyze_lambda_expression(analysis, value.expr);
                    }
                    else if constexpr (std::is_same_v< value_type, expression_pun >)
                    {
                        co_await this->co_analyze_lambda_expression(analysis, value.value);
                    }
                    else if constexpr (std::is_same_v< value_type, expression_place >)
                    {
                        co_await this->co_analyze_lambda_expression(analysis, value.at);
                        if (value.assign_init.has_value())
                        {
                            co_await this->co_analyze_lambda_expression(analysis, *value.assign_init);
                        }
                        for (auto const& arg : value.args)
                        {
                            co_await this->co_analyze_lambda_expression(analysis, arg.value);
                        }
                    }
                    else if constexpr (std::is_same_v< value_type, expression_choose >)
                    {
                        co_await this->co_analyze_lambda_expression(analysis, value.condition);
                        co_await this->co_analyze_lambda_expression(analysis, value.true_expr);
                        co_await this->co_analyze_lambda_expression(analysis, value.false_expr);
                    }
                    else if constexpr (std::is_same_v< value_type, expression_static_choose >)
                    {
                        auto eval_result = co_await this->co_eval_lambda_dry_static_expression(analysis, value.condition, type_symbol(bool_type{}));
                        if (this->static_eval_result_as_bool(eval_result))
                        {
                            co_await this->co_analyze_lambda_expression(analysis, value.true_expr);
                        }
                        else
                        {
                            co_await this->co_analyze_lambda_expression(analysis, value.false_expr);
                        }
                    }
                    else if constexpr (std::is_same_v< value_type, expression_pack_arg >)
                    {
                        co_await this->co_analyze_lambda_expression(analysis, value.index);
                    }
                    else if constexpr (std::is_same_v< value_type, expression_forward >)
                    {
                        if (value.symbol.template type_is< freebound_identifier >())
                        {
                            this->add_lambda_capture(analysis, value.symbol.template get_as< freebound_identifier >().name);
                        }
                    }
                    else if constexpr (std::is_same_v< value_type, expression_lambda >)
                    {
                        auto nested = co_await this->co_analyze_lambda_captures(value, analysis.possible_captures, analysis.static_context);
                        for (auto const& capture : nested.captures)
                        {
                            this->add_lambda_capture(analysis, capture.name);
                        }
                    }
                    co_return;
                });
            co_return;
        }

        auto co_analyze_lambda_block(lambda_capture_analysis_state& analysis, function_block const& block) -> co_type< void >
        {
            for (auto const& statement : block.statements)
            {
                co_await rpnx::apply_visitor< co_type< void > >(
                    statement,
                    [&](auto const& st) -> co_type< void >
                    {
                        using statement_type = std::decay_t< decltype(st) >;
                        if constexpr (std::is_same_v< statement_type, function_block >)
                        {
                            co_await this->co_analyze_lambda_block(analysis, st);
                        }
                        else if constexpr (std::is_same_v< statement_type, function_expression_statement >)
                        {
                            co_await this->co_analyze_lambda_expression(analysis, st.expr);
                        }
                        else if constexpr (std::is_same_v< statement_type, function_return_statement >)
                        {
                            if (st.expr.has_value())
                            {
                                co_await this->co_analyze_lambda_expression(analysis, *st.expr);
                            }
                        }
                        else if constexpr (std::is_same_v< statement_type, function_var_statement >)
                        {
                            for (auto const& arg : st.initializers)
                            {
                                co_await this->co_analyze_lambda_expression(analysis, arg.value);
                            }
                            if (st.equals_initializer.has_value())
                            {
                                co_await this->co_analyze_lambda_expression(analysis, *st.equals_initializer);
                            }
                            if (!st.static_kind.has_value())
                            {
                                auto resolved = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = this->ctx, .type = st.type});
                                analysis.local_types[st.name] = resolved.value_or(st.type);
                            }
                        }
                        else if constexpr (std::is_same_v< statement_type, function_if_statement >)
                        {
                            co_await this->co_analyze_lambda_expression(analysis, st.condition);
                            co_await this->co_analyze_lambda_block(analysis, st.then_block);
                            if (st.else_block.has_value())
                            {
                                co_await this->co_analyze_lambda_block(analysis, *st.else_block);
                            }
                        }
                        else if constexpr (std::is_same_v< statement_type, function_runtime_statement >)
                        {
                            co_await this->co_analyze_lambda_block(analysis, st.then_block);
                            if (st.else_block.has_value())
                            {
                                co_await this->co_analyze_lambda_block(analysis, *st.else_block);
                            }
                        }
                        else if constexpr (std::is_same_v< statement_type, function_static_if_statement >)
                        {
                            auto eval_result = co_await this->co_eval_lambda_dry_static_expression(analysis, st.condition, type_symbol(bool_type{}));
                            if (this->static_eval_result_as_bool(eval_result))
                            {
                                co_await this->co_analyze_lambda_block(analysis, st.then_block);
                            }
                            else if (st.else_block.has_value())
                            {
                                co_await this->co_analyze_lambda_block(analysis, *st.else_block);
                            }
                        }
                        else if constexpr (std::is_same_v< statement_type, function_static_while_statement >)
                        {
                            while (true)
                            {
                                auto eval_result = co_await this->co_eval_lambda_dry_static_expression(analysis, st.condition, type_symbol(bool_type{}));
                                if (!this->static_eval_result_as_bool(eval_result))
                                {
                                    break;
                                }
                                co_await this->co_analyze_lambda_block(analysis, st.loop_block);
                            }
                        }
                        else if constexpr (std::is_same_v< statement_type, function_static_eval_statement >)
                        {
                            co_await this->co_eval_lambda_dry_static_expression(analysis, st.expr, std::nullopt);
                        }
                        else if constexpr (std::is_same_v< statement_type, function_while_statement >)
                        {
                            co_await this->co_analyze_lambda_expression(analysis, st.condition);
                            co_await this->co_analyze_lambda_block(analysis, st.loop_block);
                        }
                        else if constexpr (std::is_same_v< statement_type, function_for_statement >)
                        {
                            if (st.init_block.has_value())
                            {
                                co_await this->co_analyze_lambda_block(analysis, *st.init_block);
                            }
                            if (st.eval_block.has_value())
                            {
                                co_await this->co_analyze_lambda_block(analysis, *st.eval_block);
                            }
                            if (st.test_condition.has_value())
                            {
                                co_await this->co_analyze_lambda_expression(analysis, *st.test_condition);
                            }
                            if (st.posttest_condition.has_value())
                            {
                                co_await this->co_analyze_lambda_expression(analysis, *st.posttest_condition);
                            }
                            if (st.step_block.has_value())
                            {
                                co_await this->co_analyze_lambda_block(analysis, *st.step_block);
                            }
                            if (st.in_expr.has_value())
                            {
                                co_await this->co_analyze_lambda_expression(analysis, *st.in_expr);
                            }
                            if (st.start_expr.has_value())
                            {
                                co_await this->co_analyze_lambda_expression(analysis, *st.start_expr);
                            }
                            if (st.end_expr.has_value())
                            {
                                co_await this->co_analyze_lambda_expression(analysis, *st.end_expr);
                            }
                            if (st.limit_expr.has_value())
                            {
                                co_await this->co_analyze_lambda_expression(analysis, *st.limit_expr);
                            }
                            if (st.filter_expr.has_value())
                            {
                                co_await this->co_analyze_lambda_expression(analysis, *st.filter_expr);
                            }
                            if (st.by_expr.has_value())
                            {
                                co_await this->co_analyze_lambda_expression(analysis, *st.by_expr);
                            }
                            if (st.from_expr.has_value())
                            {
                                co_await this->co_analyze_lambda_expression(analysis, *st.from_expr);
                            }
                            if (st.to_expr.has_value())
                            {
                                co_await this->co_analyze_lambda_expression(analysis, *st.to_expr);
                            }
                            if (st.until_expr.has_value())
                            {
                                co_await this->co_analyze_lambda_expression(analysis, *st.until_expr);
                            }
                            if (st.iter_name.has_value())
                            {
                                analysis.local_types[*st.iter_name] = type_symbol(auto_temploidic{});
                            }
                            if (st.value_name.has_value())
                            {
                                analysis.local_types[*st.value_name] = type_symbol(auto_temploidic{});
                            }
                            if (st.index_name.has_value())
                            {
                                analysis.local_types[*st.index_name] = type_symbol(auto_temploidic{});
                            }
                            if (st.item_name.has_value())
                            {
                                analysis.local_types[*st.item_name] = type_symbol(auto_temploidic{});
                            }
                            co_await this->co_analyze_lambda_block(analysis, st.loop_block);
                        }
                        else if constexpr (std::is_same_v< statement_type, function_assert_statement >)
                        {
                            co_await this->co_analyze_lambda_expression(analysis, st.condition);
                        }
                        else if constexpr (std::is_same_v< statement_type, function_place_statement >)
                        {
                            co_await this->co_analyze_lambda_expression(analysis, st.at);
                            if (st.assign_init.has_value())
                            {
                                co_await this->co_analyze_lambda_expression(analysis, *st.assign_init);
                            }
                            for (auto const& arg : st.args)
                            {
                                co_await this->co_analyze_lambda_expression(analysis, arg.value);
                            }
                        }
                        else if constexpr (std::is_same_v< statement_type, function_destroy_statement >)
                        {
                            co_await this->co_analyze_lambda_expression(analysis, st.at);
                            for (auto const& arg : st.args)
                            {
                                co_await this->co_analyze_lambda_expression(analysis, arg.value);
                            }
                        }
                        else if constexpr (std::is_same_v< statement_type, function_label_block_statement >)
                        {
                            co_await this->co_analyze_lambda_block(analysis, st.block);
                        }
                        co_return;
                    });
            }
            co_return;
        }

        auto co_analyze_lambda_captures(expression_lambda const& lambda, std::map< std::string, lambda_possible_capture > possible_captures,
                                        lambda_dry_static_context static_context) -> co_type< lambda_dry_run_result >
        {
            lambda_capture_analysis_state analysis;
            analysis.possible_captures = std::move(possible_captures);
            analysis.static_context = std::move(static_context);
            analysis.has_explicit_capture_list = lambda.has_explicit_capture_list;

            for (auto const& param : lambda.parameters)
            {
                if (param.name.has_value())
                {
                    analysis.local_types[*param.name] = param.type;
                }
                else if (param.api_name.has_value())
                {
                    analysis.local_types[*param.api_name] = param.type;
                }
            }
            for (auto const& capture : lambda.captures)
            {
                if (!analysis.possible_captures.contains(capture.name))
                {
                    throw semantic_compilation_error("Lambda capture source is not available: " + capture.name);
                }
                analysis.explicit_captures[capture.name] = capture.mode;
                this->add_lambda_capture(analysis, capture.name);
            }

            co_await this->co_analyze_lambda_block(analysis, lambda.body);
            co_return lambda_dry_run_result{
                .captures = std::move(analysis.captures),
                .environment = this->lambda_environment_from_analysis(analysis),
            };
        }

        auto make_lambda_operator_declaration(expression_lambda const& lambda) -> ast2_function_declaration
        {
            ast2_function_declaration declaration;
            declaration.header.call_parameters = lambda.parameters;
            declaration.definition.return_type = lambda.return_type.value_or(type_symbol(decay_temploidic{}));
            declaration.definition.body = lambda.body;
            declaration.location = lambda.location;
            return declaration;
        }

        auto co_publish_lambda_subqueries(std::size_t lambda_index, std::map< std::string, lambda_possible_capture > possible_captures,
                                          lambda_dry_run_result const& dry_run, ast2_function_declaration operator_declaration) -> co_type< void >
        {
            if constexpr (rpnx::querygraph::query_handler_produced_subqueries_t< handler_spec >::template contains< lambda_possible_captures_subquery >())
            {
                co_yield rpnx::querygraph::subquery_result< lambda_possible_captures_subquery >(lambda_index, std::move(possible_captures));
            }
            if constexpr (rpnx::querygraph::query_handler_produced_subqueries_t< handler_spec >::template contains< lambda_capture_set_subquery >())
            {
                std::vector< type_symbol > capture_types;
                for (auto const& capture : dry_run.captures)
                {
                    capture_types.push_back(capture.field_type);
                }
                co_yield rpnx::querygraph::subquery_result< lambda_capture_set_subquery >(lambda_index, std::move(capture_types));
            }
            if constexpr (rpnx::querygraph::query_handler_produced_subqueries_t< handler_spec >::template contains< lambda_environment_subquery >())
            {
                co_yield rpnx::querygraph::subquery_result< lambda_environment_subquery >(lambda_index, dry_run.environment);
            }
            if constexpr (rpnx::querygraph::query_handler_produced_subqueries_t< handler_spec >::template contains< lambda_operator_subquery >())
            {
                co_yield rpnx::querygraph::subquery_result< lambda_operator_subquery >(lambda_index, std::move(operator_declaration));
            }
            co_return;
        }

        /// Reuses constructor-call expression generation for STATIC/STATIC_VAR initialization.
        auto make_static_initializer_expression(function_var_statement const& st, type_symbol resolved_type) -> expression
        {
            expression_call call;
            call.callee = expression_symbol_reference{.symbol = std::move(resolved_type)};
            for (auto const& init : st.initializers)
            {
                call.args.push_back(init);
            }
            if (st.equals_initializer.has_value())
            {
                call.args.push_back(expression_arg{.name = std::string("OTHER"), .value = *st.equals_initializer});
            }
            return call;
        }

        auto co_generate(block_index& bidx, expression_rightarrow expr) -> co_type< value_index >
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
        auto co_generate_logic_and(block_index& bidx, expression_binary input) -> co_type< value_index >
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
        auto co_generate_logic_nand(block_index& bidx, expression_binary input) -> co_type< value_index >
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

        auto co_generate_logic_or(block_index& bidx, expression_binary input) -> co_type< value_index >
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

        auto co_generate_logic_nor(block_index& bidx, expression_binary input) -> co_type< value_index >
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

        auto co_generate_logic_xor(block_index& bidx, expression_binary input) -> co_type< value_index >
        {
            auto lhs = co_await co_generate_bool_expr(bidx, input.lhs);
            auto rhs = co_await co_generate_bool_expr(bidx, input.rhs);
            co_return co_await co_generate_binary(bidx, "!=", lhs, rhs);
        }

        auto co_generate_logic_nxor(block_index& bidx, expression_binary input) -> co_type< value_index >
        {
            auto lhs = co_await co_generate_bool_expr(bidx, input.lhs);
            auto rhs = co_await co_generate_bool_expr(bidx, input.rhs);
            co_return co_await co_generate_binary(bidx, "==", lhs, rhs);
        }

        auto co_generate_logic_implies(block_index& bidx, expression_binary input) -> co_type< value_index >
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

        auto co_generate_logic_implied(block_index& bidx, expression_binary input) -> co_type< value_index >
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

        auto co_generate_binary(block_index& bidx, std::string operator_str, value_index lhs, value_index rhs) -> co_type< value_index >
        {
            type_symbol lhs_type = this->current_type(bidx, lhs);
            type_symbol rhs_type = this->current_type(bidx, rhs);

            if (lhs_type.type_is< numeric_literal_reference >() && rhs_type.type_is< numeric_literal_reference >())
            {
                auto const& lhs_slot = this->state.genvalues.at(lhs);
                auto const& rhs_slot = this->state.genvalues.at(rhs);

                if (lhs_slot.template type_is< codegen_literal >() && rhs_slot.template type_is< codegen_literal >())
                {
                    auto bytes_to_string = [](std::vector< std::byte > const& bytes)
                    {
                        std::string out;
                        out.reserve(bytes.size());
                        for (std::byte b : bytes)
                        {
                            out.push_back(static_cast< char >(b));
                        }
                        return out;
                    };

                    auto lhs_value = parsers::str_to_int< std::int64_t >(bytes_to_string(lhs_slot.template get_as< codegen_literal >().value));
                    auto rhs_value = parsers::str_to_int< std::int64_t >(bytes_to_string(rhs_slot.template get_as< codegen_literal >().value));

                    if (operator_str == "==")
                    {
                        co_return this->create_bool_value(bidx, lhs_value == rhs_value);
                    }
                    if (operator_str == "!=")
                    {
                        co_return this->create_bool_value(bidx, lhs_value != rhs_value);
                    }
                    if (operator_str == "<")
                    {
                        co_return this->create_bool_value(bidx, lhs_value < rhs_value);
                    }
                    if (operator_str == "<=")
                    {
                        co_return this->create_bool_value(bidx, lhs_value <= rhs_value);
                    }
                    if (operator_str == ">")
                    {
                        co_return this->create_bool_value(bidx, lhs_value > rhs_value);
                    }
                    if (operator_str == ">=")
                    {
                        co_return this->create_bool_value(bidx, lhs_value >= rhs_value);
                    }
                }
            }

            type_symbol lhs_underlying_type = remove_ref(lhs_type);
            type_symbol rhs_underlying_type = remove_ref(rhs_type);

            type_symbol lhs_function = submember{lhs_underlying_type, "OPERATOR" + operator_str};
            type_symbol rhs_function = submember{rhs_underlying_type, "OPERATOR" + operator_str + "RHS"};
            invotype lhs_param_info{.named = {{"THIS", lhs_type}, {"OTHER", rhs_type}}};
            invotype rhs_param_info{.named = {{"THIS", rhs_type}, {"OTHER", lhs_type}}};

            auto lhs_exists_and_callable_with = co_await rpnx::querygraph::request< instanciation_query >(initialization_reference{.initializee = lhs_function, .parameters = instatype_from_invotype(lhs_param_info), .adaptations = allowed_adaptations::destination_rebinding});

            if (lhs_exists_and_callable_with)
            {
                auto lhs_args = codegen_invocation_args{.named = {{"THIS", lhs}, {"OTHER", rhs}}};
                co_return co_await co_gen_call_functum(bidx, lhs_function, lhs_args);
            }

            auto rhs_exists_and_callable_with = co_await rpnx::querygraph::request< instanciation_query >(initialization_reference{.initializee = rhs_function, .parameters = instatype_from_invotype(rhs_param_info), .adaptations = allowed_adaptations::destination_rebinding});

            if (rhs_exists_and_callable_with)
            {
                auto rhs_args = codegen_invocation_args{.named = {{"THIS", rhs}, {"OTHER", lhs}}};
                co_return co_await co_gen_call_functum(bidx, rhs_function, rhs_args);
            }

            throw semantic_compilation_error("Found neither " + to_string(lhs_function) + " callable with (" + to_string(lhs_type) + ", " + to_string(rhs_type) + ") nor " + to_string(rhs_function) + " callable with (" + to_string(rhs_type) + ", " + to_string(lhs_type) + ")");
        }

        auto co_generate(block_index& bidx, expression_binary input) -> co_type< value_index >
        {
            if (logic_operators.contains(input.operator_str))
            {
                if (input.operator_str == "&&")
                {
                    co_return co_await co_generate_logic_and(bidx, input);
                }

                if (input.operator_str == "&!")
                {
                    co_return co_await co_generate_logic_nand(bidx, input);
                }

                if (input.operator_str == "||")
                {
                    co_return co_await co_generate_logic_or(bidx, input);
                }

                if (input.operator_str == "|!")
                {
                    co_return co_await co_generate_logic_nor(bidx, input);
                }

                if (input.operator_str == "^^")
                {
                    co_return co_await co_generate_logic_xor(bidx, input);
                }

                if (input.operator_str == "^!")
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

        auto co_generate(block_index& bidx, expression_numeric_literal input) -> co_type< value_index >
        {
            auto val = this->create_numeric_literal(input.value);
            assert(val != 0);
            auto val_type = this->current_type(bidx, val);
            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                co_yield rpnx::querygraph::debug_message("Generated numeric literal {} of type {}", static_cast< std::uint64_t >(val), to_string(val_type));
            }
            co_return val;
        }

        auto co_generate(block_index& bidx, expression_string_literal input) -> co_type< value_index >
        {
            auto val = this->create_string_literal(input.value);
            assert(val != 0);
            auto val_type = this->current_type(bidx, val);
            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                co_yield rpnx::querygraph::debug_message("Generated string literal {} of type {}", static_cast< std::uint64_t >(val), to_string(val_type));
            }
            co_return val;
        }

        auto co_begin_storage_delegate(block_index& bidx, value_index storage_ref, type_symbol target_type, bool destroy_delegate) -> co_type< value_index >
        {
            co_await co_expect_storage_reference(bidx, storage_ref, true, target_type);

            auto delegate_value = this->create_local_value(target_type);
            if (destroy_delegate)
            {
                this->emit(bidx, vmir2::storage_deinit_start{.on_storage = get_local_index(storage_ref), .target_value = get_local_index(delegate_value)});
            }
            else
            {
                this->emit(bidx, vmir2::storage_init_start{.on_storage = get_local_index(storage_ref), .target_value = get_local_index(delegate_value)});
            }

            co_return delegate_value;
        }

        auto co_generate_place_expression_impl(block_index& bidx, value_index storage_ref, type_symbol target_type, std::optional< expression > const& assign_init, std::vector< expression_arg > const& args_in) -> co_type< value_index >
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
            this->emit(bidx, vmir2::storage_pun{.from_storage = get_local_index(storage_ref), .as_type = target_type, .to_reference = get_local_index(typed_ref)});

            auto result_pointer = create_local_value(ptrref_type{.target = target_type, .ptr_class = pointer_class::instance, .qual = storage_ref_type.qual});
            this->emit(bidx, vmir2::make_pointer_to{.of_index = get_local_index(typed_ref), .pointer_index = get_local_index(result_pointer)});
            co_return result_pointer;
        }

        auto co_generate_place_expression(block_index& bidx, expression const& at_expr, type_symbol const& parsed_type, std::optional< expression > const& assign_init, std::vector< expression_arg > const& args_in) -> co_type< value_index >
        {
            auto target_type = co_await this->co_resolve_type_symbol(bidx, parsed_type);
            auto storage_ref = co_await co_generate_expr(bidx, at_expr);
            co_return co_await co_generate_place_expression_impl(bidx, storage_ref, target_type, assign_init, args_in);
        }

        auto co_generate(block_index& bidx, expression_typecast input) -> co_type< value_index >
        {
            // Casts call the destination type's constructor with a named argument.
            // Bare AS prefers EXPLICIT and falls back to OTHER to preserve existing @OTHER-based casts.
            auto arg_val = co_await co_generate_expr(bidx, input.expr);

            type_symbol target_class = co_await this->co_resolve_type_symbol(bidx, input.to_type);

            if (std::optional< value_index > flagset_cast = co_await co_try_generate_flagset_to_unsigned_cast(bidx, arg_val, target_class, input.keyword); flagset_cast.has_value())
            {
                co_return *flagset_cast;
            }

            if (input.keyword.has_value())
            {
                codegen_invocation_args args;
                args.named[*input.keyword] = arg_val;
                co_return co_await co_gen_call_ctor(bidx, target_class, args);
            }

            if (auto explicit_ctor = co_await co_try_gen_call_ctor_with_named_argument(bidx, target_class, "EXPLICIT", arg_val); explicit_ctor.has_value())
            {
                co_return *explicit_ctor;
            }

            if (auto other_ctor = co_await co_try_gen_call_ctor_with_named_argument(bidx, target_class, "OTHER", arg_val); other_ctor.has_value())
            {
                co_return *other_ctor;
            }

            throw semantic_compilation_error("Cannot cast " + to_string(this->current_type(bidx, arg_val)) + " AS " + to_string(target_class));
        }

        auto co_try_generate_flagset_to_unsigned_cast(block_index& bidx, value_index arg_val, type_symbol const& target_class, std::optional< std::string > const& keyword) -> co_type< std::optional< value_index > >
        {
            if (keyword.has_value() && *keyword != "EXPLICIT")
            {
                co_return std::nullopt;
            }

            type_symbol source_type = this->current_type(bidx, arg_val);
            type_symbol source_value_type = remove_ref(source_type);
            if (co_await rpnx::querygraph::request< symbol_type_query >(source_value_type) != symbol_kind::flagset_)
            {
                co_return std::nullopt;
            }

            std::optional< std::uint64_t > target_bits;
            if (target_class.type_is< int_type >())
            {
                int_type const& target_integer = target_class.get_as< int_type >();
                if (!target_integer.has_sign)
                {
                    target_bits = target_integer.bits;
                }
            }
            else if (target_class.type_is< byte_type >())
            {
                target_bits = 8;
            }

            if (!target_bits.has_value())
            {
                co_return std::nullopt;
            }

            flagset_info const info = co_await rpnx::querygraph::request< flagset_info_query >(source_value_type);
            if (*target_bits < info.bits)
            {
                throw semantic_compilation_error("Cannot cast FLAGSET " + to_string(source_value_type) + " to narrower unsigned integer " + to_string(target_class));
            }

            value_index source_value = arg_val;
            if (is_ref(source_type))
            {
                source_value = load_reference_value(bidx, arg_val, source_value_type);
            }

            value_index result = this->create_local_value(target_class);
            this->emit(bidx, vmir2::iconv{.from = get_local_index(source_value), .to = get_local_index(result), .convtype = vmir2::conversion_class::partial});
            co_return result;
        }

        auto co_generate(block_index& bidx, expression_pun input) -> co_type< value_index >
        {
            auto storage_ref = co_await co_generate_expr(bidx, input.value);
            auto target_type = co_await this->co_resolve_type_symbol(bidx, input.as_type);
            auto storage_ref_type = co_await co_expect_storage_reference(bidx, storage_ref, false, target_type);
            auto result_ref = create_local_value(ptrref_type{.target = target_type, .ptr_class = pointer_class::ref, .qual = storage_ref_type.qual});
            this->emit(bidx, vmir2::storage_pun{.from_storage = get_local_index(storage_ref), .as_type = target_type, .to_reference = get_local_index(result_ref)});
            co_return result_ref;
        }

        auto co_generate(block_index& bidx, expression_place input) -> co_type< value_index >
        {
            co_return co_await co_generate_place_expression(bidx, input.at, input.type, input.assign_init, input.args);
        }

        auto co_generate(block_index& bidx, expression_unary_postfix input) -> co_type< value_index >
        {
            auto val = co_await co_generate_expr(bidx, input.lhs);
            co_return co_await co_generate_unary_postfix(bidx, input.operator_str, val);
        }

        auto co_generate_unary_postfix(block_index& bidx, std::string operator_str, value_index val) -> co_type< value_index >
        {
            auto oper = this->get_class_member(bidx, val, "OPERATOR" + operator_str);
            co_return co_await co_gen_call_functum(bidx, oper, codegen_invocation_args{.named = {{"THIS", val}}});
        }

        auto co_generate(block_index& bidx, expression_unary_prefix input) -> co_type< value_index >
        {
            auto val = co_await co_generate_expr(bidx, input.rhs);
            auto oper = this->get_class_member(bidx, val, "OPERATOR" + input.operator_str + "PREFIX");
            co_return co_await co_gen_call_functum(bidx, oper, codegen_invocation_args{.named = {{"THIS", val}}});
        }

        auto co_generate(block_index& bidx, expression_multibind const& what) -> co_type< value_index >
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

        auto co_generate_bool_expr(block_index& bidx, expression expr) -> co_type< value_index >
        {
            return this->co_generate_typed_expr(bidx, expr, bool_type{});
        }

        auto co_generate_typed_expr(block_index& bidx, expression expr, type_symbol target_type) -> co_type< value_index >
        {
            auto location_scope = this->scoped_source_location(get_location(expr));
            std::string expr_str = quxlang::to_string(expr);
            auto expr_val = co_await co_generate_expr(bidx, expr);
            assert(bidx == block_index(0) || this->state.blocks.at(0).terminator.has_value());

            auto type_of_expr = this->current_type(bidx, expr_val);

            if (type_of_expr == target_type)
            {
                co_return expr_val;
            }

            implicitly_convertible_to_input query;
            query.from = type_of_expr;
            query.to = target_type;

            bool convertible = co_await rpnx::querygraph::request< implicitly_convertible_to_qg_query >(query);

            if (!convertible)
            {
                throw semantic_compilation_error("Cannot convert " + quxlang::to_string(type_of_expr) + " to " + quxlang::to_string(target_type));
            }

            co_return co_await co_gen_implicit_conversion(bidx, expr_val, target_type);
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_if_statement const& st) -> co_type< void >
        {
            block_index after_block = this->generate_subblock(current_block, "if_statement_after");
            block_index condition_block = this->generate_subblock(current_block, "if_statement_condition");
            block_index if_block = this->generate_subblock(current_block, "if_block");

            this->generate_jump(current_block, condition_block);

            auto cond = co_await co_generate_bool_expr(condition_block, st.condition);

            if (!st.else_block.has_value())
            {
                {
                    auto condition_location_scope = this->scoped_source_location(get_location(st.condition));
                    this->generate_branch(cond, condition_block, if_block, after_block);
                }

                // Then
                co_await co_generate_function_block(if_block, st.then_block, "if_then");
                this->generate_jump(if_block, after_block);
            }
            else
            {
                block_index else_block = this->generate_subblock(current_block, "if_statement_else");
                {
                    auto condition_location_scope = this->scoped_source_location(get_location(st.condition));
                    this->generate_branch(cond, condition_block, if_block, else_block);
                }

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

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_unimplemented_statement const& st) -> co_type< void >
        {
            this->emit(current_block, vmir2::unimplemented{.message = st.error_message});
            co_return;
        }

        [[nodiscard]] auto find_labeled_break_target(std::string const& label_name) const -> std::optional< block_index >
        {
            for (auto it = this->state.break_controls.rbegin(); it != this->state.break_controls.rend(); ++it)
            {
                if (it->label_name == label_name)
                {
                    return it->break_target;
                }
            }
            return std::nullopt;
        }

        [[nodiscard]] auto find_labeled_continue_target(std::string const& label_name) const -> std::optional< block_index >
        {
            for (auto it = this->state.loop_controls.rbegin(); it != this->state.loop_controls.rend(); ++it)
            {
                if (it->label_name.has_value() && *it->label_name == label_name)
                {
                    return it->continue_target;
                }
            }
            return std::nullopt;
        }

        [[nodiscard]] auto goto_live_state_compatible(vmir2::slot_state const& source, vmir2::slot_state const& target) const -> bool
        {
            if (source.stage != target.stage || source.storage_valid != target.storage_valid)
            {
                return false;
            }
            if (source.delegate_of != target.delegate_of || source.destroy_delegate != target.destroy_delegate || source.array_delegate_of_initializer != target.array_delegate_of_initializer)
            {
                return false;
            }
            if (source.delegates.has_value() || target.delegates.has_value() || source.nontrivial_dtor.has_value() || target.nontrivial_dtor.has_value())
            {
                return false;
            }
            return true;
        }

        auto validate_goto_transition(block_index source, block_index target, std::string const& label_name, std::optional< source_location > const&) const -> void
        {
            auto const& source_state = this->state.blocks.at(source).current_state;
            auto const& target_state = this->state.blocks.at(target).entry_state;

            for (auto const& [idx, target_slot] : target_state)
            {
                if (!target_slot.alive())
                {
                    continue;
                }
                auto source_it = source_state.find(idx);
                if (source_it == source_state.end() || !source_it->second.alive())
                {
                    throw semantic_compilation_error("Invalid GOTO :" + label_name + ": target requires live slot " + std::to_string(idx));
                }
                if (!this->goto_live_state_compatible(source_it->second, target_slot))
                {
                    throw semantic_compilation_error("Invalid GOTO :" + label_name + ": target slot " + std::to_string(idx) + " has incompatible live state");
                }
            }
        }

        auto get_or_create_goto_label_target(std::string const& label_name, block_index current_block) -> block_index
        {
            auto target_it = this->state.goto_labels.find(label_name);
            if (target_it != this->state.goto_labels.end())
            {
                return target_it->second.target;
            }

            auto target = this->generate_subblock(current_block, "goto_label_" + label_name);
            this->state.goto_labels.emplace(label_name, goto_label_target{.target = target});
            return target;
        }

        auto validate_pending_gotos(std::string const& label_name) -> void
        {
            auto pending_it = this->state.pending_gotos.find(label_name);
            if (pending_it == this->state.pending_gotos.end())
            {
                return;
            }
            auto target = this->state.goto_labels.at(label_name).target;
            for (auto const& pending : pending_it->second)
            {
                this->validate_goto_transition(pending.source, target, label_name, pending.location);
            }
            this->state.pending_gotos.erase(pending_it);
        }

        auto validate_no_pending_gotos() -> void
        {
            if (this->state.pending_gotos.empty())
            {
                return;
            }
            auto const& [label_name, pending] = *this->state.pending_gotos.begin();
            (void)pending;
            throw semantic_compilation_error("Invalid GOTO :" + label_name + ": target label was not declared");
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_break_statement const& st) -> co_type< void >
        {
            std::optional< block_index > target;
            if (st.label_name.has_value())
            {
                target = this->find_labeled_break_target(*st.label_name);
                if (!target.has_value())
                {
                    throw semantic_compilation_error("BREAK used with unknown label: " + *st.label_name);
                }
            }
            else if (!this->state.loop_controls.empty())
            {
                target = this->state.loop_controls.back().break_target;
            }
            else
            {
                throw semantic_compilation_error("BREAK used outside a runtime loop");
            }
            this->generate_jump(current_block, *target, "break");
            co_return;
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_continue_statement const& st) -> co_type< void >
        {
            std::optional< block_index > target;
            if (st.label_name.has_value())
            {
                target = this->find_labeled_continue_target(*st.label_name);
                if (!target.has_value())
                {
                    throw semantic_compilation_error("CONTINUE used with unknown loop label: " + *st.label_name);
                }
            }
            else if (!this->state.loop_controls.empty())
            {
                target = this->state.loop_controls.back().continue_target;
            }
            else
            {
                throw semantic_compilation_error("CONTINUE used outside a runtime loop");
            }
            this->generate_jump(current_block, *target, "continue");
            co_return;
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_goto_statement const& st) -> co_type< void >
        {
            auto target = this->get_or_create_goto_label_target(st.target, current_block);
            auto const& label = this->state.goto_labels.at(st.target);
            if (label.declared)
            {
                this->validate_goto_transition(current_block, target, st.target, st.location);
            }
            else
            {
                this->state.pending_gotos[st.target].push_back(pending_goto_fixup{.source = current_block, .target = st.target, .location = st.location});
            }
            this->generate_jump(current_block, target, "goto");
            co_return;
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_label_statement const& st) -> co_type< void >
        {
            auto target = this->get_or_create_goto_label_target(st.name, current_block);
            auto& label = this->state.goto_labels.at(st.name);
            if (label.declared)
            {
                throw semantic_compilation_error("Duplicate LABEL :" + st.name);
            }

            auto label_state = this->state.blocks.at(current_block).current_state;
            auto label_lookup_values = this->state.blocks.at(current_block).lookup_values;
            this->generate_jump(current_block, target);

            auto& target_block = this->state.blocks.at(target);
            target_block.entry_state = std::move(label_state);
            target_block.current_state = target_block.entry_state;
            target_block.lookup_values = std::move(label_lookup_values);
            label.declared = true;
            label.location = st.location;
            this->validate_pending_gotos(st.name);

            current_block = target;
            co_return;
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_label_block_statement const& st) -> co_type< void >
        {
            block_index body_block = this->generate_subblock(current_block, "label_block_body");
            block_index after_block = this->generate_subblock(current_block, "label_block_after");

            this->generate_jump(current_block, body_block);
            this->state.break_controls.push_back(break_control_targets{.label_name = st.name, .break_target = after_block});
            co_await co_generate_function_block(body_block, st.block, "label_block");
            this->state.break_controls.pop_back();
            this->generate_jump(body_block, after_block);

            current_block = after_block;
            co_return;
        }

        /// Generates STATIC_EVAL by evaluating its expression during generation.
        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_static_eval_statement const& st) -> co_type< void >
        {
            (void)current_block;
            co_await this->co_eval_static_expression(st.expr, std::nullopt, static_eval_access::mutable_view);
            co_return;
        }

        /// Generates only the selected STATIC_IF branch after evaluating its condition.
        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_static_if_statement const& st) -> co_type< void >
        {
            auto eval_result = co_await this->co_eval_static_expression(st.condition, type_symbol(bool_type{}), static_eval_access::mutable_view);
            if (this->static_eval_result_as_bool(eval_result))
            {
                co_await this->co_generate_function_block(current_block, st.then_block, "static_if_then");
            }
            else if (st.else_block.has_value())
            {
                co_await this->co_generate_function_block(current_block, *st.else_block, "static_if_else");
            }
            co_return;
        }

        /// Repeats STATIC_WHILE generation while its condition evaluates to true.
        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_static_while_statement const& st) -> co_type< void >
        {
            while (true)
            {
                auto eval_result = co_await this->co_eval_static_expression(st.condition, type_symbol(bool_type{}), static_eval_access::mutable_view);
                if (!this->static_eval_result_as_bool(eval_result))
                {
                    break;
                }
                co_await this->co_generate_function_block(current_block, st.loop_block, "static_while_body");
            }
            co_return;
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_while_statement const& st) -> co_type< void >
        {
            block_index condition_block = this->generate_subblock(current_block, "while_condition");
            block_index body_block = this->generate_subblock(current_block, "while_body");
            block_index after_block = this->generate_subblock(current_block, "while_after");

            this->generate_jump(current_block, condition_block);

            auto cond = co_await co_generate_bool_expr(condition_block, st.condition);

            {
                auto condition_location_scope = this->scoped_source_location(get_location(st.condition));
                this->generate_branch(cond, condition_block, body_block, after_block);
            }

            this->state.loop_controls.push_back(loop_control_targets{.label_name = st.label_name, .break_target = after_block, .continue_target = condition_block});
            if (st.label_name.has_value())
            {
                this->state.break_controls.push_back(break_control_targets{.label_name = *st.label_name, .break_target = after_block});
            }
            co_await co_generate_function_block(body_block, st.loop_block, "while_statement");
            if (st.label_name.has_value())
            {
                this->state.break_controls.pop_back();
            }
            this->state.loop_controls.pop_back();
            this->generate_jump(body_block, condition_block);

            current_block = after_block;

            co_return;
        }

        [[nodiscard]] auto co_generate_for_clause_block(block_index& current_block, function_block const& block) -> co_type< void >
        {
            for (auto const& statement : block.statements)
            {
                co_await co_generate_fblock_statement(current_block, statement);
            }
            co_return;
        }

        [[nodiscard]] auto for_statement_has_iterator_clause(function_for_statement const& st) const -> bool
        {
            return st.iter_name.has_value() || st.index_name.has_value() || st.item_name.has_value() || st.in_expr.has_value() || st.start_expr.has_value() || st.end_expr.has_value() || st.limit_expr.has_value();
        }

        [[nodiscard]] auto for_statement_has_sequence_clause(function_for_statement const& st) const -> bool
        {
            return st.value_name.has_value() || st.by_expr.has_value() || st.from_expr.has_value() || st.to_expr.has_value() || st.until_expr.has_value();
        }

        [[nodiscard]] auto co_generate_sequence_for_statement(block_index& current_block, function_for_statement const& st) -> co_type< void >
        {
            if (st.init_block.has_value() || st.eval_block.has_value() || st.test_condition.has_value() || st.posttest_condition.has_value() || st.step_block.has_value())
            {
                throw semantic_compilation_error("FOR sequence clauses cannot be mixed with INIT, EVAL, TEST, POSTTEST, or STEP");
            }
            if (!st.from_expr.has_value())
            {
                throw semantic_compilation_error("FOR sequence loop requires FROM");
            }
            if (!st.value_name.has_value())
            {
                throw semantic_compilation_error("FOR sequence loop requires VALUE");
            }
            if (st.to_expr.has_value() == st.until_expr.has_value())
            {
                throw semantic_compilation_error("FOR sequence loop requires exactly one of TO or UNTIL");
            }

            auto outer_lookup_values = this->block(current_block).lookup_values;
            this->state.static_scopes.emplace_back();

            auto start_input = co_await co_generate_expr(current_block, *st.from_expr);
            auto sequence_type = remove_ref(this->current_type(current_block, start_input));
            if (sequence_type.template type_is< numeric_literal_reference >())
            {
                throw semantic_compilation_error("FOR sequence FROM expression must have a concrete type (Note: try casting , e.g. `FROM(1 AS I32)` or `FROM(5 AS U64)` for example)");
            }
            auto sequence_value = co_await co_gen_construct_with_target_type(current_block, start_input, sequence_type, allowed_adaptations::source_rebinding);
            this->block(current_block).lookup_values[*st.value_name] = sequence_value;

            auto end_input = co_await co_generate_expr(current_block, st.to_expr.has_value() ? *st.to_expr : *st.until_expr);
            auto end_value = co_await co_gen_construct_with_target_type(current_block, end_input, sequence_type, allowed_adaptations::source_rebinding);
            value_index by_value;
            if (st.by_expr.has_value())
            {
                auto by_input = co_await co_generate_expr(current_block, *st.by_expr);
                by_value = co_await co_gen_construct_with_target_type(current_block, by_input, sequence_type, allowed_adaptations::source_rebinding);
            }
            else
            {
                by_value = this->create_small_uint_value(current_block, 1, sequence_type);
            }

            block_index condition_block = this->generate_subblock(current_block, "for_sequence_condition");
            std::optional< block_index > filter_block;
            if (st.filter_expr.has_value())
            {
                filter_block = this->generate_subblock(current_block, "for_sequence_filter");
            }
            block_index body_block = this->generate_subblock(current_block, "for_sequence_body");
            block_index step_block = this->generate_subblock(current_block, "for_sequence_step");
            block_index after_block = this->generate_subblock(current_block, "for_sequence_after");

            this->generate_jump(current_block, condition_block);

            auto condition_value = co_await co_construct_copy(condition_block, sequence_value, sequence_type);
            auto end_compare_value = co_await co_construct_copy(condition_block, end_value, sequence_type);
            auto should_continue = co_await co_generate_binary(condition_block, st.to_expr.has_value() ? "<=" : "<", condition_value, end_compare_value);
            this->generate_branch(should_continue, condition_block, filter_block.value_or(body_block), after_block);

            if (filter_block.has_value())
            {
                auto filter_condition = co_await co_generate_bool_expr(*filter_block, *st.filter_expr);
                {
                    auto filter_location_scope = this->scoped_source_location(get_location(*st.filter_expr));
                    this->generate_branch(filter_condition, *filter_block, body_block, step_block);
                }
            }

            this->state.loop_controls.push_back(loop_control_targets{.label_name = st.label_name, .break_target = after_block, .continue_target = step_block});
            if (st.label_name.has_value())
            {
                this->state.break_controls.push_back(break_control_targets{.label_name = *st.label_name, .break_target = after_block});
            }
            co_await co_generate_function_block(body_block, st.loop_block, "for_sequence_loop");
            if (st.label_name.has_value())
            {
                this->state.break_controls.pop_back();
            }
            this->state.loop_controls.pop_back();
            this->generate_jump(body_block, step_block);

            auto old_sequence_value = co_await co_construct_copy(step_block, sequence_value, sequence_type);
            auto step_by_value = co_await co_construct_copy(step_block, by_value, sequence_type);
            auto next_sequence_value = co_await co_generate_binary(step_block, "+", old_sequence_value, step_by_value);
            co_await co_store_local_value(step_block, sequence_value, next_sequence_value, sequence_type);
            this->generate_jump(step_block, condition_block);

            current_block = after_block;
            this->block(current_block).lookup_values = std::move(outer_lookup_values);
            this->state.static_scopes.pop_back();

            co_return;
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_for_statement const& st) -> co_type< void >
        {
            if (this->for_statement_has_iterator_clause(st))
            {
                throw semantic_compilation_error("FOR iterator clauses are parsed but not yet implemented");
            }
            if (this->for_statement_has_sequence_clause(st))
            {
                co_await this->co_generate_sequence_for_statement(current_block, st);
                co_return;
            }

            auto outer_lookup_values = this->block(current_block).lookup_values;
            this->state.static_scopes.emplace_back();

            if (st.init_block.has_value())
            {
                co_await this->co_generate_for_clause_block(current_block, *st.init_block);
            }
            if (st.eval_block.has_value())
            {
                co_await this->co_generate_for_clause_block(current_block, *st.eval_block);
            }

            block_index after_block = this->generate_subblock(current_block, "for_after");
            block_index body_block = this->generate_subblock(current_block, "for_body");
            block_index next_iteration_block = body_block;
            std::optional< block_index > condition_block;
            std::optional< block_index > filter_block;
            std::optional< block_index > posttest_block;
            std::optional< block_index > step_block;

            if (st.test_condition.has_value())
            {
                condition_block = this->generate_subblock(current_block, "for_condition");
                next_iteration_block = *condition_block;
            }
            if (st.filter_expr.has_value())
            {
                filter_block = this->generate_subblock(current_block, "for_filter");
                if (!condition_block.has_value())
                {
                    next_iteration_block = *filter_block;
                }
            }
            if (st.posttest_condition.has_value())
            {
                posttest_block = this->generate_subblock(current_block, "for_posttest");
            }
            if (st.step_block.has_value())
            {
                step_block = this->generate_subblock(current_block, "for_step");
            }

            block_index continue_target = posttest_block.value_or(step_block.value_or(next_iteration_block));

            if (condition_block.has_value())
            {
                this->generate_jump(current_block, *condition_block);
                auto cond = co_await co_generate_bool_expr(*condition_block, *st.test_condition);
                {
                    auto condition_location_scope = this->scoped_source_location(get_location(*st.test_condition));
                    this->generate_branch(cond, *condition_block, filter_block.value_or(body_block), after_block);
                }
            }
            else
            {
                block_index entry_block = filter_block.value_or(body_block);
                this->generate_jump(current_block, entry_block);
            }

            if (filter_block.has_value())
            {
                auto filter_condition = co_await co_generate_bool_expr(*filter_block, *st.filter_expr);
                {
                    auto filter_location_scope = this->scoped_source_location(get_location(*st.filter_expr));
                    this->generate_branch(filter_condition, *filter_block, body_block, continue_target);
                }
            }

            this->state.loop_controls.push_back(loop_control_targets{.label_name = st.label_name, .break_target = after_block, .continue_target = continue_target});
            if (st.label_name.has_value())
            {
                this->state.break_controls.push_back(break_control_targets{.label_name = *st.label_name, .break_target = after_block});
            }
            co_await co_generate_function_block(body_block, st.loop_block, "for_loop");
            if (st.label_name.has_value())
            {
                this->state.break_controls.pop_back();
            }
            this->state.loop_controls.pop_back();

            this->generate_jump(body_block, continue_target);

            if (posttest_block.has_value())
            {
                block_index posttest_true_block = step_block.value_or(next_iteration_block);
                auto cond = co_await co_generate_bool_expr(*posttest_block, *st.posttest_condition);
                {
                    auto condition_location_scope = this->scoped_source_location(get_location(*st.posttest_condition));
                    this->generate_branch(cond, *posttest_block, posttest_true_block, after_block);
                }
            }

            if (step_block.has_value())
            {
                block_index generated_step_block = *step_block;
                co_await co_generate_function_block(generated_step_block, *st.step_block, "for_step");
                this->generate_jump(generated_step_block, next_iteration_block);
            }

            current_block = after_block;
            this->block(current_block).lookup_values = std::move(outer_lookup_values);
            this->state.static_scopes.pop_back();

            co_return;
        }

        auto generate_branch(value_index condition, block_index from, block_index true_branch, block_index false_branch) -> void
        {
            if (this->state.blocks.at(from).terminator.has_value())
            {
                throw compiler_bug("Cannot branch from a block that already has a terminator");
            }
            this->set_terminator(from, vmir2::branch{.condition = get_local_index(condition), .target_true = block_index(true_branch), .target_false = block_index(false_branch)});
        }

        auto generate_runtime_constexpr(block_index from, block_index constexpr_branch, block_index native_branch) -> void
        {
            if (this->state.blocks.at(from).terminator.has_value())
            {
                throw compiler_bug("Cannot branch from a block that already has a terminator");
            }
            this->set_terminator(from, vmir2::runtime_constexpr{.target_constexpr = block_index(constexpr_branch), .target_native = block_index(native_branch)});
        }

        auto block(block_index blk) -> codegen_block&
        {
            return this->state.blocks.at(blk);
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_expression_statement const& st) -> co_type< void >
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

        [[nodiscard]] auto co_generate_void_expr(block_index& bidx, expression const& expr) -> co_type< value_index >
        {
            return co_generate_expr(bidx, expr);
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_block const& st) -> co_type< void >
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

            rpnx::apply_visitor< void >(this->block(it).terminator.value(),
                                        [&](auto const& term)
                                        {
                                            using T = std::remove_cvref_t< decltype(term) >;
                                            if constexpr (std::is_same_v< T, vmir2::branch >)
                                            {
                                                generate_survivor_local_chain(it, term.target_true, end, survivor);
                                                generate_survivor_local_chain(it, term.target_false, end, survivor);
                                            }
                                            else if constexpr (std::is_same_v< T, vmir2::runtime_constexpr >)
                                            {
                                                generate_survivor_local_chain(it, term.target_constexpr, end, survivor);
                                                generate_survivor_local_chain(it, term.target_native, end, survivor);
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

        /// Evaluates and records a function-local STATIC or STATIC_VAR declaration.
        [[nodiscard]] auto co_generate_static_var_statement(block_index& current_block, function_var_statement const& st) -> co_type< void >
        {
            if (this->state.static_scopes.empty())
            {
                throw compiler_bug("STATIC/STATIC_VAR used without a static scope");
            }
            if (this->find_visible_static_binding(st.name).has_value())
            {
                throw semantic_compilation_error("duplicate visible static local: " + st.name);
            }
            if (this->local_value_direct_lookup(current_block, st.name).has_value())
            {
                throw semantic_compilation_error("static local conflicts with visible runtime local: " + st.name);
            }

            type_symbol var_type = co_await this->co_resolve_type_symbol(current_block, st.type);
            auto initializer = this->make_static_initializer_expression(st, var_type);
            auto eval_result = co_await this->co_eval_static_expression(std::move(initializer), var_type, static_eval_access::mutable_view);

            auto generation = ++this->state.next_static_generation[st.name];
            auto state_symbol = static_local_ref{.functanoid = this->ctx, .name = st.name, .generation = generation};
            std::optional< std::uint64_t > mutation_result_id;
            QUXLANG_COMPILER_BUG_IF(!st.static_kind.has_value(), "Static local generator received non-static VAR");
            if (*st.static_kind == function_static_kind::mutable_)
            {
                mutation_result_id = this->state.next_static_result_id++;
            }
            auto primary_result_it = eval_result.values.find(constexpr_primary_result_id);
            QUXLANG_COMPILER_BUG_IF(primary_result_it == eval_result.values.end(), "Static initializer did not produce a primary result");

            codegen_static binding{
                .type = var_type,
                .value = primary_result_it->second,
                .mutation_result_id = mutation_result_id,
            };
            this->state.statics[state_symbol] = std::move(binding);
            this->state.static_scopes.back().bindings[st.name] = state_symbol;
            co_return;
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_var_statement const& st) -> co_type< void >
        {
            if (st.static_kind.has_value())
            {
                co_await this->co_generate_static_var_statement(current_block, st);
                co_return;
            }

            std::string type_str = quxlang::to_string(st.type);
            std::string context_str = quxlang::to_string(ctx);

            if (typeis< auto_temploidic >(st.type))
            {
                if (!st.initializers.empty() || !st.equals_initializer.has_value())
                {
                    throw semantic_compilation_error("AUTO variables require a single := initializer");
                }

                block_index new_expr_block = this->generate_subblock(current_block, "auto_var_new");
                block_index after_block = this->generate_subblock(current_block, "auto_var_after");

                this->generate_jump(current_block, new_expr_block);
                current_block = new_expr_block;

                value_index init_idx = co_await co_generate_expr(new_expr_block, *st.equals_initializer);
                type_symbol var_type = this->current_type(new_expr_block, init_idx);

                if (typeis< attached_type_reference >(var_type))
                {
                    value_index copied_binding = co_await this->co_copy_attached_binding(new_expr_block, init_idx, var_type);
                    this->block(new_expr_block).lookup_values[st.name] = copied_binding;

                    this->generate_jump(new_expr_block, after_block);
                    codegen_binding const& binding = this->state.genvalues.at(copied_binding).template get_as< codegen_binding >();
                    if (binding.bound_value != value_index(0))
                    {
                        this->generate_survivor_local(new_expr_block, after_block, get_local_index(binding.bound_value));
                    }
                    this->generate_survivor_lookup(new_expr_block, after_block, st.name);
                    current_block = after_block;
                    co_return;
                }

                value_index idx = this->generate_variable_local(new_expr_block, st.name, var_type);
                codegen_invocation_args args;
                args.named["THIS"] = idx;
                args.named["OTHER"] = init_idx;

                auto ctor = submember{.of = var_type, .name = "CONSTRUCTOR"};
                co_await this->co_gen_call_functum(new_expr_block, ctor, args);
                auto class_default_dtor = co_await rpnx::querygraph::request< class_default_dtor_query >(var_type);
                if (class_default_dtor)
                {
                    if (!state.non_trivial_dtors.contains(var_type))
                    {
                        state.non_trivial_dtors[var_type] = class_default_dtor.value();
                    }
                }

                vmir2::slot_state new_state = state.blocks.at(new_expr_block).current_state[get_local_index(idx)];
                assert(new_state.valid());

                this->generate_jump(new_expr_block, after_block);
                this->generate_survivor_local(new_expr_block, after_block, get_local_index(idx));
                this->generate_survivor_lookup(new_expr_block, after_block, st.name);
                current_block = after_block;
                co_return;
            }

            type_symbol var_type = co_await this->co_resolve_type_symbol(current_block, st.type);

            if (typeis< attached_type_reference >(var_type))
            {
                if (!st.initializers.empty() || !st.equals_initializer.has_value())
                {
                    throw semantic_compilation_error("Attached binding variables require a single := initializer");
                }

                block_index new_expr_block = this->generate_subblock(current_block, "binding_var_new");
                block_index after_block = this->generate_subblock(current_block, "binding_var_after");

                this->generate_jump(current_block, new_expr_block);
                current_block = new_expr_block;

                value_index init_idx = co_await co_generate_expr(new_expr_block, *st.equals_initializer);
                value_index copied_binding = co_await this->co_copy_attached_binding(new_expr_block, init_idx, var_type);
                this->block(new_expr_block).lookup_values[st.name] = copied_binding;

                this->generate_jump(new_expr_block, after_block);
                codegen_binding const& binding = this->state.genvalues.at(copied_binding).template get_as< codegen_binding >();
                if (binding.bound_value != value_index(0))
                {
                    this->generate_survivor_local(new_expr_block, after_block, get_local_index(binding.bound_value));
                }
                this->generate_survivor_lookup(new_expr_block, after_block, st.name);
                current_block = after_block;
                co_return;
            }

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

            for (auto const& init : st.initializers)
            {
                auto init_idx = co_await co_generate_expr(new_expr_block, init.value);
                if (init.name.has_value())
                {
                    args.named[*init.name] = init_idx;
                }
                else
                {
                    args.positional.push_back(init_idx);
                }
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
                    throw semantic_compilation_error("STORAGE variables do not support direct initializers");
                }
                this->emit(new_expr_block, vmir2::storage_init{.storage = get_local_index(idx)});
            }
            else
            {
                auto ctor = submember{.of = var_type, .name = "CONSTRUCTOR"};
                co_await this->co_gen_call_functum(new_expr_block, ctor, args);
                auto class_default_dtor = co_await rpnx::querygraph::request< class_default_dtor_query >(var_type);
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

        auto co_generate(block_index& bidx, expression_thisdot_reference what) -> co_type< value_index >
        {
            auto this_reference = freebound_identifier{"THIS"};
            auto value = co_await this->co_lookup_symbol(bidx, this_reference);
            if (!value)
            {
                throw semantic_compilation_error("Cannot find " + to_string(this_reference));
            }
            auto field = co_await this->co_generate_dot_access(bidx, *value, what.field_name);
            co_return field;
        }

        auto co_generate(block_index& bidx, expression_dotreference what) -> co_type< value_index >
        {
            auto parent = co_await co_generate_expr(bidx, what.lhs);
            co_return co_await co_generate_dot_access(bidx, parent, what.field_name, std::move(what.template_arguments));
        }

        auto co_generate_dot_access(block_index& bidx, value_index base, std::string field_name, std::vector< expression_arg > template_arguments = {}) -> co_type< value_index >
        {
            auto base_type = this->current_type(bidx, base);
            std::string base_type_str = quxlang::to_string(base_type);
            auto base_type_noref = quxlang::remove_ref(base_type);

            std::string base_type_noref_string = quxlang::to_string(base_type_noref);
            symbol_kind const base_kind = co_await rpnx::querygraph::request< symbol_type_query >(base_type_noref);

            if (base_kind == symbol_kind::flagset_)
            {
                flagset_info const info = co_await rpnx::querygraph::request< flagset_info_query >(base_type_noref);
                for (flagset_value_info const& flag : info.values)
                {
                    if (flag.name != field_name)
                    {
                        continue;
                    }

                    value_index base_value = base;
                    if (is_ref(base_type))
                    {
                        base_value = this->create_local_value(base_type_noref);
                        this->emit(bidx, vmir2::load_from_ref{.from_reference = get_local_index(base), .to_value = get_local_index(base_value)});
                    }

                    auto load_mask_value = [&](block_index& load_block) -> value_index
                    {
                        value_index mask_value = this->create_local_value(base_type_noref);
                        vmir2::load_const_int load_mask;
                        load_mask.target = get_local_index(mask_value);
                        load_mask.value = std::to_string(flag.mask);
                        this->emit(load_block, load_mask);
                        return mask_value;
                    };

                    value_index mask_for_and = load_mask_value(bidx);
                    value_index and_value = this->create_local_value(base_type_noref);
                    this->emit(bidx, vmir2::bitwise_and{.a = get_local_index(base_value), .b = get_local_index(mask_for_and), .result = get_local_index(and_value)});

                    value_index mask_for_compare = load_mask_value(bidx);
                    value_index result = this->create_local_value(bool_type{});
                    this->emit(bidx, vmir2::cmp_eq{.a = get_local_index(and_value), .b = get_local_index(mask_for_compare), .result = get_local_index(result)});
                    co_return result;
                }
            }

            // First try to find a field with this name
            if (base_kind == symbol_kind::class_)
            {
                class_layout layout = co_await rpnx::querygraph::request< class_layout_query >(base_type_noref);

                // std::string base_type_str = to_string(base_type);

                for (class_field_info const& field : layout.fields)
                {
                    if (field.name == field_name)
                    {
                        if (typeis< attached_type_reference >(field.type))
                        {
                            attached_type_reference const& attached = as< attached_type_reference >(field.type);
                            if (typeis< void_type >(attached.carrying_type))
                            {
                                co_return this->create_binding(value_index(0), attached.attached_symbol);
                            }

                            vmir2::access_field access;
                            access.base_index = get_local_index(base);
                            access.field_name = field.name;
                            type_symbol carrier_ref_type = recast_reference(base_type.template get_as< ptrref_type >(), attached.carrying_type);
                            auto carrier_idx = create_local_value(carrier_ref_type);
                            access.store_index = get_local_index(carrier_idx);
                            this->emit(bidx, access);
                            co_return this->create_binding(carrier_idx, attached.attached_symbol);
                        }

                        vmir2::access_field access;
                        access.base_index = get_local_index(base);
                        access.field_name = field.name;
                        type_symbol result_ref_type = recast_reference(base_type.template get_as< ptrref_type >(), field.type);
                        auto result_idx = create_local_value(result_ref_type);
                        access.store_index = get_local_index(result_idx);
                        // co_yield rpnx::querygraph::debug_message("Created field access {} for {} in {}", access.store_index, field_name, to_string(base_type));

                        this->emit(bidx, access);
                        co_return result_idx;
                    }
                }
            }

            // If no field is found, look for a member function
            auto member_func = submember{.of = base_type_noref, .name = field_name};
            type_symbol lookup_target = member_func;
            if (!template_arguments.empty())
            {
                lookup_target = initialization_reference{.initializee = member_func, .context = ctx, .arguments = std::move(template_arguments)};
            }
            auto lookup_result = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = ctx, .type = std::move(lookup_target)});

            if (lookup_result)
            {
                // Create a binding to the member function with the base object
                auto binding = create_binding(base, lookup_result.value());
                if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
                {
                    co_yield rpnx::querygraph::debug_message("Created member function binding {} for {} in {}", static_cast< std::uint64_t >(binding), field_name, to_string(base_type));
                }
                co_return binding;
            }

            throw semantic_compilation_error("Cannot find field " + field_name + " in " + to_string(base_type));
        }

        auto co_generate_builtin_ctor(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();
            block_index current_block = block_index(0);
            auto cls = func.temploid.templexoid.get_as< submember >().of;

            if (co_await rpnx::querygraph::request< symbol_type_query >(cls) == symbol_kind::interface_)
            {
                if (!co_await this->co_try_emit_interface_builtin_from_locals(current_block, func))
                {
                    throw compiler_bug("Interface constructor routine is not implemented: " + quxlang::to_string(func));
                }
                co_await co_generate_builtin_return(current_block);
                co_await co_generate_dtor_references();
                co_return get_result();
            }

            symbol_kind const cls_kind = co_await rpnx::querygraph::request< symbol_type_query >(cls);
            if (cls_kind == symbol_kind::enum_ || cls_kind == symbol_kind::flagset_)
            {
                auto thisidx = this->local_value_direct_lookup(current_block, "THIS");
                if (!thisidx.has_value())
                {
                    throw compiler_bug("Nominal integer constructor is missing THIS");
                }
                if (cls_kind == symbol_kind::flagset_)
                {
                    this->emit(current_block, vmir2::load_const_zero{.target = get_local_index(*thisidx)});
                }
                else
                {
                    enum_info const info = co_await rpnx::querygraph::request< enum_info_query >(cls);
                    if (!info.default_value_name.has_value())
                    {
                        throw semantic_compilation_error("ENUM is not default constructible: " + to_string(cls));
                    }
                    bool found = false;
                    for (enum_value_info const& value : info.values)
                    {
                        if (value.name == *info.default_value_name)
                        {
                            vmir2::load_const_int load_default;
                            load_default.target = get_local_index(*thisidx);
                            load_default.value = std::to_string(value.value);
                            this->emit(current_block, load_default);
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        throw compiler_bug("ENUM default value was not present in enum_info");
                    }
                }
                co_await co_generate_builtin_return(current_block);
                co_await co_generate_dtor_references();
                co_return get_result();
            }

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

        auto co_generate_interface_get_impl(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();
            block_index current_block = block_index(0);

            if (!typeis< subsymbol >(func.temploid.templexoid))
            {
                throw compiler_bug("GET_INTERFACE_IMPL must be a subsymbol functanoid");
            }

            subsymbol const& function_symbol = as< subsymbol >(func.temploid.templexoid);
            type_symbol implementation_type = function_symbol.of;
            type_symbol interface_type = co_await rpnx::querygraph::request< implementation_interface_type_query >(implementation_type);
            std::map< interface_slot_key, type_symbol > functions = co_await rpnx::querygraph::request< implementation_function_map_query >(implementation_type);
            std::optional< value_index > return_value = this->local_value_direct_lookup(current_block, "RETURN");
            if (!return_value.has_value())
            {
                throw compiler_bug("GET_INTERFACE_IMPL has no RETURN parameter");
            }

            this->emit(current_block, vmir2::interface_init{
                                          .target = get_local_index(*return_value),
                                          .interface_type = std::move(interface_type),
                                          .functions = std::move(functions),
                                          .is_default = false,
                                      });

            co_await co_generate_builtin_return(current_block);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_swap(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
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

        auto co_generate_builtin_global_init(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();
            block_index current_block = block_index(0);

            auto global_symbol = func.temploid.templexoid.get_as< submember >().of;
            auto global_type = co_await rpnx::querygraph::request< variable_type_query >(global_symbol);
            auto decl = co_await rpnx::querygraph::request< symboid_query >(global_symbol);
            if (!typeis< ast2_variable_declaration >(decl))
            {
                throw compiler_bug("Global variable declaration not found");
            }

            auto const& variable_decl = as< ast2_variable_declaration >(decl);
            auto storage_ref = (co_await this->co_lookup_symbol(current_block, freebound_identifier{"STORAGE"})).value();

            if (co_await rpnx::querygraph::request< global_is_string_static_query >(global_symbol))
            {
                auto string_value = co_await rpnx::querygraph::request< string_static_value_query >(global_symbol);
                auto storage_delegate = co_await co_begin_storage_delegate(current_block, storage_ref, global_type, false);
                this->emit(current_block, vmir2::load_const_value{
                                              .target = get_local_index(storage_delegate),
                                              .value = std::move(string_value.bytes),
                                          });
                co_await co_generate_builtin_return(current_block);
                co_await co_generate_dtor_references();
                co_return get_result();
            }

            if (co_await rpnx::querygraph::request< global_is_serialoid_static_query >(global_symbol))
            {
                auto serialoid_value = co_await rpnx::querygraph::request< serialoid_static_value_query >(global_symbol);
                auto data_value = this->create_local_value(readonly_constant{.kind = constant_kind::data});
                this->emit(current_block, vmir2::load_const_value{
                                              .target = get_local_index(data_value),
                                              .value = std::move(serialoid_value.bytes),
                                          });
                auto begin_functum = submember{.of = type_symbol(readonly_constant{.kind = constant_kind::data}), .name = "BEGIN"};
                auto input_iter = co_await this->co_gen_call_functum(current_block, begin_functum, codegen_invocation_args{.named = {{"THIS", data_value}}});

                auto constructor = submember{.of = global_type, .name = "CONSTRUCTOR"};
                invotype deserialize_ctor_call;
                deserialize_ctor_call.named["THIS"] = nvalue_slot{.target = global_type};
                deserialize_ctor_call.named["DESERIALIZE_INPUT_ITERATOR"] = ptrref_type{.target = byte_type{}, .ptr_class = pointer_class::array, .qual = qualifier::constant};
                initialization_reference deserialize_ctor_probe{
                    .initializee = constructor,
                    .parameters = instatype_from_invotype(deserialize_ctor_call),
                    .adaptations = allowed_adaptations::destination_rebinding,
                };
                auto deserialize_ctor = co_await rpnx::querygraph::request< instanciation_query >(deserialize_ctor_probe);

                if (deserialize_ctor.has_value())
                {
                    auto storage_delegate = co_await co_begin_storage_delegate(current_block, storage_ref, global_type, false);
                    co_await this->co_gen_call_functum(current_block, constructor, codegen_invocation_args{.named = {{"THIS", storage_delegate}, {"DESERIALIZE_INPUT_ITERATOR", input_iter}}});
                    co_await co_generate_builtin_return(current_block);
                    co_await co_generate_dtor_references();
                    co_return get_result();
                }

                invotype default_ctor_call;
                default_ctor_call.named["THIS"] = nvalue_slot{.target = global_type};
                initialization_reference default_ctor_probe{
                    .initializee = constructor,
                    .parameters = instatype_from_invotype(default_ctor_call),
                    .adaptations = allowed_adaptations::destination_rebinding,
                };
                auto default_ctor = co_await rpnx::querygraph::request< instanciation_query >(default_ctor_probe);
                if (!default_ctor.has_value())
                {
                    throw semantic_compilation_error("serialoid STATIC requires a deserialize constructor or default constructor plus DESERIALIZE: " + quxlang::to_string(global_symbol));
                }

                auto initialized = co_await co_generate_place_expression_impl(current_block, storage_ref, global_type, std::nullopt, {});
                auto object_ref = this->create_local_value(make_mref(global_type));
                this->emit(current_block, vmir2::dereference_pointer{.from_pointer = get_local_index(initialized), .to_reference = get_local_index(object_ref)});
                auto deserialize_functum = submember{.of = global_type, .name = "DESERIALIZE"};
                co_await this->co_gen_call_functum(current_block, deserialize_functum, codegen_invocation_args{.named = {{"THIS", object_ref}, {"INPUT_ITERATOR", input_iter}}});
                co_await co_generate_builtin_return(current_block);
                co_await co_generate_dtor_references();
                co_return get_result();
            }

            auto ignored = co_await co_generate_place_expression_impl(current_block, storage_ref, global_type, variable_decl.init_expr, variable_decl.init_args);
            (void)ignored;

            co_await co_generate_builtin_return(current_block);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_global_get_reference(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();

            auto entry_block = block_index(0);
            auto global_symbol = func.temploid.templexoid.get_as< submember >().of;
            auto global_type = co_await rpnx::querygraph::request< variable_type_query >(global_symbol);

            if (co_await rpnx::querygraph::request< global_is_antestatal_static_query >(global_symbol))
            {
                auto result_ref = this->create_local_value(make_cref(global_type));
                this->emit(entry_block, vmir2::get_antestatal_ref{
                                            .symbol = global_symbol,
                                            .target_ref = get_local_index(result_ref),
                                        });

                co_await this->co_return_value(entry_block, result_ref);
                co_await co_generate_dtor_references();
                co_return get_result();
            }

            bool const is_serialoid_static = co_await rpnx::querygraph::request< global_is_serialoid_static_query >(global_symbol);
            bool const is_string_static = co_await rpnx::querygraph::request< global_is_string_static_query >(global_symbol);

            storage global_storage_type;
            global_storage_type.storable_types.insert(global_type);

            auto lock_value = create_local_value(initguard_lock_type{});
            auto initialized_block = this->generate_subblock(entry_block, "global_already_initialized");
            auto acquire_block = this->generate_subblock(entry_block, "global_acquired");

            this->set_terminator(entry_block, vmir2::initguard_try_acquire{
                .symbol = global_symbol,
                .target_lock = get_local_index(lock_value),
                .target_acquired = acquire_block,
                .target_already_initialized = initialized_block,
            });

            vmir2::slot_state lock_state;
            lock_state.stage = vmir2::slot_stage::full;
            lock_state.storage_valid = true;
            this->block(acquire_block).entry_state[get_local_index(lock_value)] = lock_state;
            this->block(acquire_block).current_state[get_local_index(lock_value)] = lock_state;

            auto emit_return_from_storage = [&](block_index& current_block) -> co_type< void >
            {
                auto storage_ref = this->create_local_value(make_mref(global_storage_type));
                this->emit(current_block, vmir2::get_global_storage{
                                              .symbol = global_symbol,
                                              .target_ref = get_local_index(storage_ref),
                                          });

                auto result_ref = this->create_local_value((is_serialoid_static || is_string_static) ? make_cref(global_type) : make_mref(global_type));
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

        auto co_generate_builtin_access_member(instanciation_reference const& func, std::string const& member_name) -> co_type< quxlang::vmir2::functanoid_routine3 >
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

        /// Selects an unsigned integer type for UINTANY and LEB128 arithmetic, widening BYTE and narrow integers to at least 16 bits.
        auto uintany_work_type(type_symbol const& value_type) -> type_symbol
        {
            if (value_type.type_is< byte_type >())
            {
                return int_type{.bits = 16, .has_sign = false};
            }
            if (!value_type.type_is< int_type >())
            {
                throw semantic_compilation_error("UINTANY serialization requires an unsigned integer value");
            }

            auto const& int_value_type = value_type.get_as< int_type >();
            if (int_value_type.has_sign)
            {
                throw semantic_compilation_error("UINTANY serialization requires an unsigned integer value");
            }

            if (int_value_type.bits < 16)
            {
                return int_type{.bits = 16, .has_sign = false};
            }
            return value_type;
        }

        /// Emits a load_from_ref instruction and returns the new local containing the referenced value.
        auto load_reference_value(block_index& current_block, value_index ref, type_symbol const& value_type) -> value_index
        {
            auto value = this->create_local_value(value_type);
            this->emit(current_block, vmir2::load_from_ref{.from_reference = get_local_index(ref), .to_value = get_local_index(value)});
            return value;
        }

        /// Emits a zero-initialized local of the requested type and returns its value index.
        auto load_zero_value(block_index& current_block, type_symbol const& value_type) -> value_index
        {
            auto value = this->create_local_value(value_type);
            this->emit(current_block, vmir2::load_const_zero{.target = get_local_index(value)});
            return value;
        }

        /// Constructs a new local by invoking the type's normal copy constructor machinery on the source value.
        auto co_construct_copy(block_index& current_block, value_index value, type_symbol const& value_type) -> co_type< value_index >
        {
            auto copy = this->create_local_value(value_type);
            auto ctor = submember{.of = value_type, .name = "CONSTRUCTOR"};
            auto other = value;
            if (!is_ref(this->current_type(current_block, other)))
            {
                other = this->create_reference(current_block, other, make_cref(value_type));
            }
            co_await co_gen_call_functum(current_block, ctor, codegen_invocation_args{.named = {{"OTHER", other}, {"THIS", copy}}}, allowed_adaptations::source_rebinding);
            co_return copy;
        }

        /// Returns the original value when it already has the target type, otherwise emits an integer conversion to the target type.
        auto convert_value(block_index& current_block, value_index value, type_symbol const& target_type) -> value_index
        {
            if (this->current_type(current_block, value) == target_type)
            {
                return value;
            }

            auto converted = this->create_local_value(target_type);
            this->emit(current_block, vmir2::iconv{.from = get_local_index(value), .to = get_local_index(converted), .convtype = vmir2::conversion_class::partial});
            return converted;
        }

        /// Emits a small unsigned integer constant into a new local of the requested target type.
        auto create_small_uint_value(block_index& current_block, std::uint64_t value, type_symbol const& target_type) -> value_index
        {
            auto result = this->create_local_value(target_type);
            vmir2::load_const_int instr;
            instr.value = std::to_string(value);
            instr.target = get_local_index(result);
            this->emit(current_block, instr);
            return result;
        }

        /// Stores a value into an existing local by taking a mutable reference and using the ordinary assignment operator path.
        auto co_store_local_value(block_index& current_block, value_index local, value_index value, type_symbol const& value_type) -> co_type< void >
        {
            auto local_ref = this->create_reference(current_block, local, make_mref(value_type));
            co_await co_generate_binary(current_block, ":=", local_ref, value);
            co_return;
        }


        /// Writes one byte through the OUTPUT_ITERATOR argument using the language-level ++, ->, and := iterator operations.
        auto co_emit_output_byte(block_index& current_block, value_index byte_value) -> co_type< void >
        {
            auto outit_ref = (co_await this->co_lookup_symbol(current_block, freebound_identifier{"OUTPUT_ITERATOR"})).value();
            auto incr = co_await co_generate_unary_postfix(current_block, "++", outit_ref);
            auto outit_deref = co_await co_generate_unary_postfix(current_block, "->", incr);
            co_await co_generate_binary(current_block, ":=", outit_deref, byte_value);
            co_return;
        }

        /// Reads one byte from the INPUT_ITERATOR argument using the language-level ++ and -> iterator operations.
        auto co_read_input_byte(block_index& current_block) -> co_type< value_index >
        {
            auto input_iter = co_await this->co_lookup_symbol(current_block, freebound_identifier{"INPUT_ITERATOR"});
            if (!input_iter.has_value())
            {
                throw compiler_bug("Missing INPUT_ITERATOR argument");
            }

            auto input_iter_arg = *input_iter;
            auto input_iter_type = this->current_type(current_block, input_iter_arg);
            if (!is_ref(input_iter_type))
            {
                input_iter_arg = this->create_reference(current_block, input_iter_arg, make_mref(input_iter_type));
            }

            auto incr = co_await co_generate_unary_postfix(current_block, "++", input_iter_arg);
            auto input_deref = co_await co_generate_unary_postfix(current_block, "->", incr);
            co_return load_reference_value(current_block, input_deref, byte_type{});
        }

        /// Generates the shared unsigned variable-length integer serializer; offset_long_encodings selects UINTANY offset continuation semantics instead of plain LEB128.
        auto co_generate_builtin_serialize_varuint(instanciation_reference const& func, bool offset_long_encodings) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();

            auto current_block = block_index(0);
            auto value_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"VALUE"});
            if (!value_ref.has_value())
            {
                throw compiler_bug("Missing varuint VALUE argument");
            }

            type_symbol value_ref_type = this->state.params.named.at("VALUE").type;
            if (!typeis< ptrref_type >(value_ref_type) || as< ptrref_type >(value_ref_type).ptr_class != pointer_class::ref)
            {
                throw compiler_bug("varuint VALUE argument must be a reference");
            }

            type_symbol value_type = as< ptrref_type >(value_ref_type).target;
            auto work_type = uintany_work_type(value_type);
            auto input_value = load_reference_value(current_block, *value_ref, value_type);
            auto remaining = convert_value(current_block, input_value, work_type);

            auto condition_block = this->generate_subblock(current_block, "varuint_serialize_condition");
            auto continue_block = this->generate_subblock(current_block, "varuint_serialize_continue");
            auto final_block = this->generate_subblock(current_block, "varuint_serialize_final");

            this->generate_jump(current_block, condition_block);

            auto condition_value = load_reference_value(condition_block, this->create_reference(condition_block, remaining, make_cref(work_type)), work_type);
            auto condition_128 = create_small_uint_value(condition_block, 128, work_type);
            auto is_final_byte = co_await co_generate_binary(condition_block, "<", condition_value, condition_128);
            this->generate_branch(is_final_byte, condition_block, final_block, continue_block);

            auto continue_value = co_await co_construct_copy(continue_block, remaining, work_type);
            auto continue_128_for_payload = create_small_uint_value(continue_block, 128, work_type);
            auto payload = co_await co_generate_binary(continue_block, "%", continue_value, continue_128_for_payload);
            auto continue_128_for_tag = create_small_uint_value(continue_block, 128, work_type);
            auto continued_payload = co_await co_generate_binary(continue_block, "#||", payload, continue_128_for_tag);
            auto continued_byte = convert_value(continue_block, continued_payload, byte_type{});
            co_await co_emit_output_byte(continue_block, continued_byte);

            auto continue_value_for_quotient = co_await co_construct_copy(continue_block, remaining, work_type);
            auto continue_128_for_quotient = create_small_uint_value(continue_block, 128, work_type);
            auto quotient = co_await co_generate_binary(continue_block, "/", continue_value_for_quotient, continue_128_for_quotient);
            auto next_remaining = quotient;
            if (offset_long_encodings)
            {
                auto one = create_small_uint_value(continue_block, 1, work_type);
                next_remaining = co_await co_generate_binary(continue_block, "-", quotient, one);
            }
            co_await co_store_local_value(continue_block, remaining, next_remaining, work_type);
            this->generate_jump(continue_block, condition_block);

            auto final_value = co_await co_construct_copy(final_block, remaining, work_type);
            auto final_byte = convert_value(final_block, final_value, byte_type{});
            co_await co_emit_output_byte(final_block, final_byte);

            auto outit_ref = co_await this->co_lookup_symbol(final_block, freebound_identifier{"OUTPUT_ITERATOR"});
            if (!outit_ref.has_value())
            {
                throw compiler_bug("Missing varuint OUTPUT_ITERATOR argument");
            }

            co_await co_return_value(final_block, *outit_ref);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        /// Generates SERIALIZE_UINTANY, which writes UINTANY bytes to OUTPUT_ITERATOR and returns that iterator.
        auto co_generate_builtin_serialize_uintany(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            co_return co_await co_generate_builtin_serialize_varuint(func, true);
        }

        /// Generates SERIALIZE_LEB128, which writes unsigned LEB128 bytes to OUTPUT_ITERATOR and returns that iterator.
        auto co_generate_builtin_serialize_leb128(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            co_return co_await co_generate_builtin_serialize_varuint(func, false);
        }

        /// Generates the shared unsigned variable-length integer deserializer; offset_long_encodings selects UINTANY offset continuation semantics instead of plain LEB128.
        auto co_generate_builtin_deserialize_varuint(instanciation_reference const& func, bool offset_long_encodings) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();

            auto current_block = block_index(0);
            auto value_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"VALUE"});
            if (!value_ref.has_value())
            {
                throw compiler_bug("Missing varuint VALUE argument");
            }

            type_symbol value_ref_type = this->state.params.named.at("VALUE").type;
            if (!typeis< ptrref_type >(value_ref_type) || as< ptrref_type >(value_ref_type).ptr_class != pointer_class::ref)
            {
                throw compiler_bug("varuint VALUE argument must be a reference");
            }

            type_symbol value_type = as< ptrref_type >(value_ref_type).target;
            auto work_type = uintany_work_type(value_type);
            auto uintptr_type = co_await rpnx::querygraph::request< uintpointer_type_query >({});

            auto accum = load_zero_value(current_block, work_type);
            auto shift = load_zero_value(current_block, uintptr_type);

            auto loop_block = this->generate_subblock(current_block, "varuint_deserialize_loop");
            auto continue_block = this->generate_subblock(current_block, "varuint_deserialize_continue");
            auto final_block = this->generate_subblock(current_block, "varuint_deserialize_final");

            this->generate_jump(current_block, loop_block);

            auto byte_value = co_await co_read_input_byte(loop_block);
            auto byte_as_work = convert_value(loop_block, byte_value, work_type);
            auto mask_127 = create_small_uint_value(loop_block, 127, work_type);
            auto byte_payload_value = co_await co_construct_copy(loop_block, byte_as_work, work_type);
            auto payload_value = co_await co_generate_binary(loop_block, "#&&", byte_payload_value, mask_127);
            auto shift_value = co_await co_construct_copy(loop_block, shift, uintptr_type);
            auto shifted_payload = co_await co_generate_binary(loop_block, "#++", payload_value, shift_value);
            auto accum_value = co_await co_construct_copy(loop_block, accum, work_type);
            auto accum_with_payload = co_await co_generate_binary(loop_block, "+", accum_value, shifted_payload);
            co_await co_store_local_value(loop_block, accum, accum_with_payload, work_type);

            auto mask_128 = create_small_uint_value(loop_block, 128, work_type);
            auto byte_continuation_value = co_await co_construct_copy(loop_block, byte_as_work, work_type);
            auto continuation_value = co_await co_generate_binary(loop_block, "#&&", byte_continuation_value, mask_128);
            auto zero_value = load_zero_value(loop_block, work_type);
            auto has_continuation = co_await co_generate_binary(loop_block, "!=", continuation_value, zero_value);
            this->generate_branch(has_continuation, loop_block, continue_block, final_block);

            auto old_shift = co_await co_construct_copy(continue_block, shift, uintptr_type);
            auto seven = create_small_uint_value(continue_block, 7, uintptr_type);
            auto next_shift = co_await co_generate_binary(continue_block, "+", old_shift, seven);
            co_await co_store_local_value(continue_block, shift, next_shift, uintptr_type);

            if (offset_long_encodings)
            {
                auto one_value = create_small_uint_value(continue_block, 1, work_type);
                auto shift_for_offset = co_await co_construct_copy(continue_block, shift, uintptr_type);
                auto offset_add = co_await co_generate_binary(continue_block, "#++", one_value, shift_for_offset);
                auto accum_before_offset = co_await co_construct_copy(continue_block, accum, work_type);
                auto accum_with_offset = co_await co_generate_binary(continue_block, "+", accum_before_offset, offset_add);
                co_await co_store_local_value(continue_block, accum, accum_with_offset, work_type);
            }
            this->generate_jump(continue_block, loop_block);

            auto final_accum = co_await co_construct_copy(final_block, accum, work_type);
            auto final_value = convert_value(final_block, final_accum, value_type);
            this->emit(final_block, vmir2::store_to_ref{.from_value = get_local_index(final_value), .to_reference = get_local_index(*value_ref)});

            auto input_iter = co_await this->co_lookup_symbol(final_block, freebound_identifier{"INPUT_ITERATOR"});
            if (!input_iter.has_value())
            {
                throw compiler_bug("Missing varuint INPUT_ITERATOR argument");
            }

            co_await co_return_value(final_block, *input_iter);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        /// Generates DESERIALIZE_UINTANY, which reads UINTANY bytes from INPUT_ITERATOR into VALUE and returns that iterator.
        auto co_generate_builtin_deserialize_uintany(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            co_return co_await co_generate_builtin_deserialize_varuint(func, true);
        }

        /// Generates DESERIALIZE_LEB128, which reads unsigned LEB128 bytes from INPUT_ITERATOR into VALUE and returns that iterator.
        auto co_generate_builtin_deserialize_leb128(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            co_return co_await co_generate_builtin_deserialize_varuint(func, false);
        }

        auto co_generate_builtin_serialize(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));

            type_symbol class_type = func.temploid.templexoid.get_as< submember >().of;

            if (class_type.type_is< int_type >() || class_type.type_is< byte_type >())
            {
                co_return co_await this->co_generate_builtin_serialize_int(func);
            }
            if (class_type.type_is< bool_type >())
            {
                co_return co_await this->co_generate_builtin_serialize_bool(func);
            }
            if (class_type.type_is< float_type >())
            {
                co_return co_await this->co_generate_builtin_serialize_float(func);
            }
            symbol_kind const class_kind = co_await rpnx::querygraph::request< symbol_type_query >(class_type);
            if (class_kind == symbol_kind::enum_ || class_kind == symbol_kind::flagset_)
            {
                co_return co_await this->co_generate_builtin_serialize_nominal_integer(func);
            }
            co_return co_await this->co_generate_builtin_serialize_struct(func);
        }

        auto co_generate_builtin_deserialize(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));

            type_symbol class_type = func.temploid.templexoid.get_as< submember >().of;

            if (class_type.type_is< int_type >() || class_type.type_is< byte_type >())
            {
                co_return co_await this->co_generate_builtin_deserialize_int(func);
            }
            if (class_type.type_is< bool_type >())
            {
                co_return co_await this->co_generate_builtin_deserialize_bool(func);
            }
            if (class_type.type_is< float_type >())
            {
                co_return co_await this->co_generate_builtin_deserialize_float(func);
            }
            symbol_kind const class_kind = co_await rpnx::querygraph::request< symbol_type_query >(class_type);
            if (class_kind == symbol_kind::enum_ || class_kind == symbol_kind::flagset_)
            {
                co_return co_await this->co_generate_builtin_deserialize_nominal_integer(func);
            }
            co_return co_await this->co_generate_builtin_deserialize_struct(func);
        }

        auto co_nominal_integer_storage_bytes(type_symbol const& class_type) -> co_type< std::uint64_t >
        {
            symbol_kind const class_kind = co_await rpnx::querygraph::request< symbol_type_query >(class_type);
            if (class_kind == symbol_kind::enum_)
            {
                enum_info const info = co_await rpnx::querygraph::request< enum_info_query >(class_type);
                co_return info.storage_bytes;
            }
            if (class_kind == symbol_kind::flagset_)
            {
                flagset_info const info = co_await rpnx::querygraph::request< flagset_info_query >(class_type);
                co_return info.storage_bytes;
            }
            throw compiler_bug("Expected nominal integer type");
        }

        auto create_nominal_integer_const(block_index& current_block, type_symbol const& class_type, std::uint64_t value) -> value_index
        {
            value_index result = this->create_local_value(class_type);
            vmir2::load_const_int load;
            load.target = get_local_index(result);
            load.value = std::to_string(value);
            this->emit(current_block, load);
            return result;
        }

        auto load_nominal_integer_copy(block_index& current_block, type_symbol const& class_type, value_index raw_value) -> value_index
        {
            value_index raw_ref = this->create_reference(current_block, raw_value, make_cref(class_type));
            return load_reference_value(current_block, raw_ref, class_type);
        }

        auto emit_nominal_integer_assert_false(block_index& current_block, std::string message) -> void
        {
            value_index false_value = this->create_bool_value(current_block, false);
            this->emit(current_block, vmir2::assert_instr{.condition = get_local_index(false_value), .message = std::move(message)});
        }

        auto co_emit_nominal_padding_validation(block_index& current_block, type_symbol const& storage_type, value_index storage_value, std::uint64_t bits, std::uint64_t storage_bytes) -> co_type< void >
        {
            std::uint64_t const storage_bits = storage_bytes * 8;
            std::uint64_t const value_mask = bits >= 64 ? std::numeric_limits< std::uint64_t >::max() : ((std::uint64_t{1} << bits) - 1);
            std::uint64_t const storage_mask = storage_bits >= 64 ? std::numeric_limits< std::uint64_t >::max() : ((std::uint64_t{1} << storage_bits) - 1);
            std::uint64_t const padding_mask = storage_mask & ~value_mask;
            if (padding_mask == 0)
            {
                co_return;
            }

            value_index raw_copy = load_nominal_integer_copy(current_block, storage_type, storage_value);
            value_index mask_value = create_nominal_integer_const(current_block, storage_type, padding_mask);
            value_index masked_value = this->create_local_value(storage_type);
            this->emit(current_block, vmir2::bitwise_and{.a = get_local_index(raw_copy), .b = get_local_index(mask_value), .result = get_local_index(masked_value)});

            value_index zero_value = create_nominal_integer_const(current_block, storage_type, 0);
            value_index condition = this->create_local_value(bool_type{});
            this->emit(current_block, vmir2::cmp_eq{.a = get_local_index(masked_value), .b = get_local_index(zero_value), .result = get_local_index(condition)});
            this->emit(current_block, vmir2::assert_instr{.condition = get_local_index(condition), .message = "nominal integer deserialization padding bits are nonzero"});
            co_return;
        }

        auto co_emit_enum_deserialize_validation(block_index& current_block, type_symbol const& class_type, value_index raw_value, enum_info const& info) -> co_type< void >
        {
            if (info.allow_unknown && info.reserved_ranges.empty())
            {
                co_return;
            }

            block_index invalid_block = this->generate_subblock(current_block, "enum_deserialize_invalid");
            block_index valid_block = this->generate_subblock(current_block, "enum_deserialize_valid");
            block_index check_block = current_block;

            for (enum_reserved_range_info const& reserved : info.reserved_ranges)
            {
                block_index ge_true_block = this->generate_subblock(check_block, "enum_deserialize_reserved_upper_check");
                block_index next_range_block = this->generate_subblock(check_block, "enum_deserialize_reserved_next");

                value_index ge_raw = load_nominal_integer_copy(check_block, class_type, raw_value);
                value_index from_value = create_nominal_integer_const(check_block, class_type, reserved.from);
                value_index ge_condition = this->create_local_value(bool_type{});
                this->emit(check_block, vmir2::cmp_ge{.a = get_local_index(ge_raw), .b = get_local_index(from_value), .result = get_local_index(ge_condition)});
                this->generate_branch(ge_condition, check_block, ge_true_block, next_range_block);

                value_index le_raw = load_nominal_integer_copy(ge_true_block, class_type, raw_value);
                value_index to_value = create_nominal_integer_const(ge_true_block, class_type, reserved.to);
                value_index le_condition = this->create_local_value(bool_type{});
                this->emit(ge_true_block, vmir2::cmp_ge{.a = get_local_index(to_value), .b = get_local_index(le_raw), .result = get_local_index(le_condition)});
                this->generate_branch(le_condition, ge_true_block, invalid_block, next_range_block);

                check_block = next_range_block;
            }

            if (info.allow_unknown)
            {
                this->generate_jump(check_block, valid_block);
            }
            else
            {
                for (enum_value_info const& value : info.values)
                {
                    block_index next_value_block = this->generate_subblock(check_block, "enum_deserialize_value_next");
                    value_index raw_copy = load_nominal_integer_copy(check_block, class_type, raw_value);
                    value_index expected_value = create_nominal_integer_const(check_block, class_type, value.value);
                    value_index match_condition = this->create_local_value(bool_type{});
                    this->emit(check_block, vmir2::cmp_eq{.a = get_local_index(raw_copy), .b = get_local_index(expected_value), .result = get_local_index(match_condition)});
                    this->generate_branch(match_condition, check_block, valid_block, next_value_block);
                    check_block = next_value_block;
                }
                this->generate_jump(check_block, invalid_block);
            }

            emit_nominal_integer_assert_false(invalid_block, "ENUM deserialization rejected reserved or unknown value");
            this->generate_jump(invalid_block, valid_block);
            current_block = valid_block;
            co_return;
        }

        auto co_generate_builtin_serialize_nominal_integer(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            type_symbol class_type = func.temploid.templexoid.get_as< submember >().of;
            co_await co_generate_arg_info(func);
            this->generate_entry_block();

            block_index current_block = block_index(0);
            auto this_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"THIS"});
            if (!this_ref.has_value())
            {
                throw compiler_bug("Missing builtin SERIALIZE THIS");
            }

            value_index raw_value = load_reference_value(current_block, *this_ref, class_type);
            std::uint64_t const byte_count = co_await co_nominal_integer_storage_bytes(class_type);
            for (std::uint64_t i = 0; i < byte_count; ++i)
            {
                value_index raw_ref = this->create_reference(current_block, raw_value, make_cref(class_type));
                value_index byte_value = this->create_local_value(byte_type{});
                this->emit(current_block, vmir2::get_value_byte{.source_reference = get_local_index(raw_ref), .offset = i, .result = get_local_index(byte_value)});
                co_await co_emit_output_byte(current_block, byte_value);
            }

            auto outit_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"OUTPUT_ITERATOR"});
            if (!outit_ref.has_value())
            {
                throw compiler_bug("Missing builtin SERIALIZE iterator");
            }
            co_await co_return_value(current_block, *outit_ref);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_deserialize_nominal_integer(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            type_symbol class_type = func.temploid.templexoid.get_as< submember >().of;
            co_await co_generate_arg_info(func);
            this->generate_entry_block();

            block_index current_block = block_index(0);
            auto this_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"THIS"});
            if (!this_ref.has_value())
            {
                throw compiler_bug("Missing builtin DESERIALIZE THIS");
            }

            value_index raw_value = load_zero_value(current_block, class_type);
            std::uint64_t const byte_count = co_await co_nominal_integer_storage_bytes(class_type);
            type_symbol storage_type = int_type{.bits = byte_count * 8, .has_sign = false};
            value_index storage_value = load_zero_value(current_block, storage_type);
            for (std::uint64_t i = 0; i < byte_count; ++i)
            {
                value_index byte_value = co_await co_read_input_byte(current_block);
                value_index byte_ref = this->create_reference(current_block, byte_value, make_cref(byte_type{}));
                value_index byte_copy = load_reference_value(current_block, byte_ref, byte_type{});

                value_index raw_ref = this->create_reference(current_block, raw_value, make_mref(class_type));
                this->emit(current_block, vmir2::set_value_byte{.target_reference = get_local_index(raw_ref), .offset = i, .value = get_local_index(byte_value)});

                value_index storage_ref = this->create_reference(current_block, storage_value, make_mref(storage_type));
                this->emit(current_block, vmir2::set_value_byte{.target_reference = get_local_index(storage_ref), .offset = i, .value = get_local_index(byte_copy)});
            }

            symbol_kind const class_kind = co_await rpnx::querygraph::request< symbol_type_query >(class_type);
            if (class_kind == symbol_kind::enum_)
            {
                enum_info const info = co_await rpnx::querygraph::request< enum_info_query >(class_type);
                co_await co_emit_enum_deserialize_validation(current_block, class_type, raw_value, info);
                co_await co_emit_nominal_padding_validation(current_block, storage_type, storage_value, info.bits, info.storage_bytes);
            }
            else if (class_kind == symbol_kind::flagset_)
            {
                flagset_info const info = co_await rpnx::querygraph::request< flagset_info_query >(class_type);
                co_await co_emit_nominal_padding_validation(current_block, storage_type, storage_value, info.bits, info.storage_bytes);
            }
            else
            {
                throw compiler_bug("Expected nominal integer type");
            }

            this->emit(current_block, vmir2::store_to_ref{.from_value = get_local_index(raw_value), .to_reference = get_local_index(*this_ref)});
            auto input_iter = co_await this->co_lookup_symbol(current_block, freebound_identifier{"INPUT_ITERATOR"});
            if (!input_iter.has_value())
            {
                throw compiler_bug("Missing builtin DESERIALIZE iterator");
            }
            co_await co_return_value(current_block, *input_iter);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_serialize_int(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
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

            co_await co_gen_call_functum(current_block, val_ctor, codegen_invocation_args{.named = {{"OTHER", this_ref.value()}, {"THIS", copy_val}}}, allowed_adaptations::source_rebinding);

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
                auto outit_ref = (co_await this->co_lookup_symbol(current_block, freebound_identifier{"OUTPUT_ITERATOR"})).value();
                auto incr = co_await co_generate_unary_postfix(current_block, "++", outit_ref);

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

                if (i + 8 >= bits)
                {
                    continue;
                }

                {
                    copymutref = this->create_reference(current_block, copy_val, class_mreftype);
                    copy_val_copy = this->create_local_value(class_type);
                    vmir2::load_from_ref lfr;
                    lfr.from_reference = get_local_index(copymutref);
                    lfr.to_value = get_local_index(copy_val_copy);
                    this->emit(current_block, lfr);
                }

                auto eight = create_numeric_literal("8");
                auto shifted_val = co_await co_generate_binary(current_block, "#--", copy_val_copy, eight);
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

        auto co_generate_builtin_serialize_bool(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();

            auto current_block = block_index(0);
            auto this_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"THIS"});
            auto outit_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"OUTPUT_ITERATOR"});

            if (!this_ref.has_value() || !outit_ref.has_value())
            {
                throw compiler_bug("Missing builtin SERIALIZE arguments");
            }

            auto byteval = this->create_local_value(byte_type{});
            {
                vmir2::load_from_ref lfr;
                lfr.from_reference = get_local_index(*this_ref);
                lfr.to_value = get_local_index(byteval);
                this->emit(current_block, lfr);
            }

            auto outit_mut_ref = (co_await this->co_lookup_symbol(current_block, freebound_identifier{"OUTPUT_ITERATOR"})).value();
            auto incr = co_await co_generate_unary_postfix(current_block, "++", outit_mut_ref);
            auto outit_deref = co_await co_generate_unary_postfix(current_block, "->", incr);
            co_await co_generate_binary(current_block, ":=", outit_deref, byteval);

            outit_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"OUTPUT_ITERATOR"});
            if (!outit_ref.has_value())
            {
                throw compiler_bug("Missing builtin SERIALIZE iterator");
            }

            co_await co_return_value(current_block, *outit_ref);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_serialize_float(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            type_symbol class_type = func.temploid.templexoid.get_as< submember >().of;
            QUXLANG_COMPILER_BUG_IF(!class_type.type_is< float_type >(), "Expected float type for float serialization");

            co_await co_generate_arg_info(func);
            this->generate_entry_block();

            auto current_block = block_index(0);
            auto this_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"THIS"});
            if (!this_ref.has_value())
            {
                throw compiler_bug("Missing builtin SERIALIZE THIS");
            }

            auto raw_value = load_reference_value(current_block, *this_ref, class_type);
            auto canonical_value = this->create_local_value(class_type);
            this->emit(current_block, vmir2::canonicalize_float{.source = get_local_index(raw_value), .result = get_local_index(canonical_value)});

            std::size_t const byte_count = (class_type.as< float_type >().bits + 7) / 8;
            for (std::size_t i = 0; i < byte_count; ++i)
            {
                auto canonical_ref = this->create_reference(current_block, canonical_value, make_cref(class_type));
                auto byte_value = this->create_local_value(byte_type{});
                this->emit(current_block, vmir2::get_value_byte{.source_reference = get_local_index(canonical_ref), .offset = i, .result = get_local_index(byte_value)});
                co_await co_emit_output_byte(current_block, byte_value);
            }

            auto outit_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"OUTPUT_ITERATOR"});
            if (!outit_ref.has_value())
            {
                throw compiler_bug("Missing builtin SERIALIZE iterator");
            }

            co_await co_return_value(current_block, *outit_ref);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_serialize_struct(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            auto class_type = func.temploid.templexoid.get_as< submember >().of;
            co_await co_generate_arg_info(func);
            this->generate_entry_block();

            auto current_block = block_index(0);
            auto current_iter = co_await this->co_lookup_symbol(current_block, freebound_identifier{"OUTPUT_ITERATOR"});

            if (!current_iter.has_value())
            {
                throw compiler_bug("Missing builtin SERIALIZE arguments");
            }

            auto const& fields = co_await rpnx::querygraph::request< class_field_list_query >(class_type);
            for (class_field const& fld : fields)
            {
                std::optional< type_symbol > field_storage_type = storage_type_for_attached_field(fld.type);
                if (!field_storage_type.has_value())
                {
                    continue;
                }
                auto this_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"THIS"});
                if (!this_ref.has_value())
                {
                    throw compiler_bug("Missing builtin SERIALIZE THIS");
                }
                auto field_ref = co_await this->co_generate_dot_access(current_block, *this_ref, fld.name);
                if (typeis< attached_type_reference >(fld.type))
                {
                    field_ref = this->attached_binding_carrier_value(current_block, field_ref, fld.type);
                }
                auto field_serialize_functum = submember{.of = *field_storage_type, .name = "SERIALIZE"};
                current_iter = co_await this->co_gen_call_functum(current_block, field_serialize_functum, codegen_invocation_args{.named = {{"THIS", field_ref}, {"OUTPUT_ITERATOR", *current_iter}}});
            }

            co_await co_return_value(current_block, *current_iter);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_deserialize_bool(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();

            auto current_block = block_index(0);
            auto this_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"THIS"});
            auto input_iter = co_await this->co_lookup_symbol(current_block, freebound_identifier{"INPUT_ITERATOR"});

            if (!this_ref.has_value() || !input_iter.has_value())
            {
                throw compiler_bug("Missing builtin DESERIALIZE arguments");
            }

            auto byteval = co_await co_read_input_byte(current_block);

            {
                vmir2::store_to_ref str;
                str.from_value = get_local_index(byteval);
                str.to_reference = get_local_index(*this_ref);
                this->emit(current_block, str);
            }

            input_iter = co_await this->co_lookup_symbol(current_block, freebound_identifier{"INPUT_ITERATOR"});
            if (!input_iter.has_value())
            {
                throw compiler_bug("Missing builtin DESERIALIZE iterator");
            }

            co_await co_return_value(current_block, *input_iter);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_deserialize_int(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            auto class_type = func.temploid.templexoid.get_as< submember >().of;
            assert(class_type.type_is< int_type >() || class_type.type_is< byte_type >());

            co_await co_generate_arg_info(func);
            this->generate_entry_block();

            auto current_block = block_index(0);
            auto this_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"THIS"});
            auto input_iter = co_await this->co_lookup_symbol(current_block, freebound_identifier{"INPUT_ITERATOR"});

            if (!this_ref.has_value() || !input_iter.has_value())
            {
                throw compiler_bug("Missing builtin DESERIALIZE arguments");
            }

            std::size_t bits = class_type.type_is< byte_type >() ? 8 : class_type.as< int_type >().bits;
            std::size_t rounded_bits = ((bits + 7) / 8) * 8;
            auto accum_type = class_type.type_is< byte_type >() ? type_symbol(byte_type{}) : type_symbol(int_type{.bits = rounded_bits, .has_sign = false});
            auto accum = this->create_local_value(accum_type);
            this->emit(current_block, vmir2::load_const_zero{.target = get_local_index(accum)});

            auto accum_mref_type = make_mref(accum_type);

            for (std::size_t i = 0; i < rounded_bits; i += 8)
            {
                auto byteval = co_await co_read_input_byte(current_block);

                value_index chunk = byteval;
                if (!accum_type.type_is< byte_type >())
                {
                    chunk = this->create_local_value(accum_type);
                    vmir2::iconv icv;
                    icv.convtype = vmir2::conversion_class::partial;
                    icv.from = get_local_index(byteval);
                    icv.to = get_local_index(chunk);
                    this->emit(current_block, icv);
                }

                value_index shifted_chunk = chunk;
                if (i != 0)
                {
                    auto shift_amount = create_numeric_literal(std::to_string(i));
                    shifted_chunk = co_await co_generate_binary(current_block, "#++", chunk, shift_amount);
                }

                auto accum_ref = this->create_reference(current_block, accum, accum_mref_type);
                auto accum_copy = this->create_local_value(accum_type);
                {
                    vmir2::load_from_ref lfr;
                    lfr.from_reference = get_local_index(accum_ref);
                    lfr.to_value = get_local_index(accum_copy);
                    this->emit(current_block, lfr);
                }

                auto merged = co_await co_generate_binary(current_block, "#||", accum_copy, shifted_chunk);
                accum_ref = this->create_reference(current_block, accum, accum_mref_type);
                co_await co_generate_binary(current_block, ":=", accum_ref, merged);

                input_iter = co_await this->co_lookup_symbol(current_block, freebound_identifier{"INPUT_ITERATOR"});
                if (!input_iter.has_value())
                {
                    throw compiler_bug("Missing builtin DESERIALIZE iterator");
                }
            }

            value_index result_value = accum;
            if (accum_type != class_type)
            {
                auto converted = this->create_local_value(class_type);
                vmir2::iconv icv;
                icv.convtype = vmir2::conversion_class::partial;
                icv.from = get_local_index(accum);
                icv.to = get_local_index(converted);
                this->emit(current_block, icv);
                result_value = converted;
            }

            {
                vmir2::store_to_ref str;
                str.from_value = get_local_index(result_value);
                str.to_reference = get_local_index(*this_ref);
                this->emit(current_block, str);
            }

            input_iter = co_await this->co_lookup_symbol(current_block, freebound_identifier{"INPUT_ITERATOR"});
            if (!input_iter.has_value())
            {
                throw compiler_bug("Missing builtin DESERIALIZE iterator");
            }

            co_await co_return_value(current_block, *input_iter);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_deserialize_float(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            auto class_type = func.temploid.templexoid.get_as< submember >().of;
            QUXLANG_COMPILER_BUG_IF(!class_type.type_is< float_type >(), "Expected float type for float deserialization");

            co_await co_generate_arg_info(func);
            this->generate_entry_block();

            auto current_block = block_index(0);
            auto this_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"THIS"});
            if (!this_ref.has_value())
            {
                throw compiler_bug("Missing builtin DESERIALIZE THIS");
            }

            auto raw_value = load_zero_value(current_block, class_type);
            std::size_t const byte_count = (class_type.as< float_type >().bits + 7) / 8;
            for (std::size_t i = 0; i < byte_count; ++i)
            {
                auto byte_value = co_await co_read_input_byte(current_block);
                auto raw_ref = this->create_reference(current_block, raw_value, make_mref(class_type));
                this->emit(current_block, vmir2::set_value_byte{.target_reference = get_local_index(raw_ref), .offset = i, .value = get_local_index(byte_value)});
            }

            auto canonical_value = this->create_local_value(class_type);
            this->emit(current_block, vmir2::canonicalize_float{.source = get_local_index(raw_value), .result = get_local_index(canonical_value)});
            this->emit(current_block, vmir2::store_to_ref{.from_value = get_local_index(canonical_value), .to_reference = get_local_index(*this_ref)});

            auto input_iter = co_await this->co_lookup_symbol(current_block, freebound_identifier{"INPUT_ITERATOR"});
            if (!input_iter.has_value())
            {
                throw compiler_bug("Missing builtin DESERIALIZE iterator");
            }

            co_await co_return_value(current_block, *input_iter);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_deserialize_struct(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            auto class_type = func.temploid.templexoid.get_as< submember >().of;
            co_await co_generate_arg_info(func);
            this->generate_entry_block();

            auto current_block = block_index(0);
            auto current_iter = co_await this->co_lookup_symbol(current_block, freebound_identifier{"INPUT_ITERATOR"});

            if (!current_iter.has_value())
            {
                throw compiler_bug("Missing builtin DESERIALIZE arguments");
            }

            auto const& fields = co_await rpnx::querygraph::request< class_field_list_query >(class_type);
            for (class_field const& fld : fields)
            {
                std::optional< type_symbol > field_storage_type = storage_type_for_attached_field(fld.type);
                if (!field_storage_type.has_value())
                {
                    continue;
                }
                auto this_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"THIS"});
                if (!this_ref.has_value())
                {
                    throw compiler_bug("Missing builtin DESERIALIZE THIS");
                }
                auto field_ref = co_await this->co_generate_dot_access(current_block, *this_ref, fld.name);
                if (typeis< attached_type_reference >(fld.type))
                {
                    field_ref = this->attached_binding_carrier_value(current_block, field_ref, fld.type);
                }
                auto field_deserialize_functum = submember{.of = *field_storage_type, .name = "DESERIALIZE"};
                current_iter = co_await this->co_gen_call_functum(current_block, field_deserialize_functum, codegen_invocation_args{.named = {{"THIS", field_ref}, {"INPUT_ITERATOR", *current_iter}}});
            }

            co_await co_return_value(current_block, *current_iter);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_datatype_compare(instanciation_reference const& func, bool invert) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            auto class_type = func.temploid.templexoid.get_as< submember >().of;
            co_await co_generate_arg_info(func);
            this->generate_entry_block();

            auto current_block = block_index(0);
            auto const& fields = co_await rpnx::querygraph::request< class_field_list_query >(class_type);

            for (class_field const& fld : fields)
            {
                std::optional< type_symbol > field_storage_type = storage_type_for_attached_field(fld.type);
                if (!field_storage_type.has_value())
                {
                    continue;
                }
                auto this_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"THIS"});
                auto other_ref = co_await this->co_lookup_symbol(current_block, freebound_identifier{"OTHER"});

                if (!this_ref.has_value() || !other_ref.has_value())
                {
                    throw compiler_bug("Missing builtin datatype comparison arguments");
                }

                auto this_field = co_await this->co_generate_dot_access(current_block, *this_ref, fld.name);
                auto other_field = co_await this->co_generate_dot_access(current_block, *other_ref, fld.name);
                if (typeis< attached_type_reference >(fld.type))
                {
                    this_field = this->attached_binding_carrier_value(current_block, this_field, fld.type);
                    other_field = this->attached_binding_carrier_value(current_block, other_field, fld.type);
                }
                auto fields_equal = co_await this->co_generate_binary(current_block, "==", this_field, other_field);

                auto match_block = this->generate_subblock(current_block, "datatype_compare_match");
                auto mismatch_block = this->generate_subblock(current_block, "datatype_compare_mismatch");
                this->generate_branch(fields_equal, current_block, match_block, mismatch_block);
                this->kill_entry_value(match_block, fields_equal);
                this->kill_entry_value(mismatch_block, fields_equal);

                auto mismatch_result = this->create_bool_value(mismatch_block, invert);
                co_await this->co_return_value(mismatch_block, mismatch_result);

                current_block = match_block;
            }

            auto final_result = this->create_bool_value(current_block, !invert);
            co_await this->co_return_value(current_block, final_result);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_copy_ctor(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();
            block_index current_block = block_index(0);

            if (typeis< submember >(func.temploid.templexoid))
            {
                submember const& member = as< submember >(func.temploid.templexoid);
                if (co_await rpnx::querygraph::request< symbol_type_query >(member.of) == symbol_kind::interface_)
                {
                    if (!co_await this->co_try_emit_interface_builtin_from_locals(current_block, func))
                    {
                        throw compiler_bug("Interface copy constructor routine is not implemented: " + quxlang::to_string(func));
                    }
                    co_await co_generate_builtin_return(current_block);
                    co_await co_generate_dtor_references();
                    co_return get_result();
                }
            }

            auto thisidx = this->local_value_direct_lookup(current_block, "THIS");
            auto otheridx = this->local_value_direct_lookup(current_block, "OTHER");
            if (thisidx.has_value() && otheridx.has_value())
            {
                codegen_invocation_args args;
                args.named["THIS"] = *thisidx;
                args.named["OTHER"] = *otheridx;
                instatype concrete_params = co_await rpnx::querygraph::request< instanciation_concrete_params_query >(func);
                invotype concrete_call = invotype_from_instatype(concrete_params);
                if (typeis< submember >(func.temploid.templexoid))
                {
                    submember const& member = as< submember >(func.temploid.templexoid);
                    symbol_kind const member_kind = co_await rpnx::querygraph::request< symbol_type_query >(member.of);
                    if (member_kind == symbol_kind::enum_ || member_kind == symbol_kind::flagset_)
                    {
                        this->emit(current_block, vmir2::load_from_ref{.from_reference = get_local_index(*otheridx), .to_value = get_local_index(*thisidx)});
                        co_await co_generate_builtin_return(current_block);
                        co_await co_generate_dtor_references();
                        co_return get_result();
                    }
                }
                if (auto intrinsic = this->intrinsic_instruction(func, concrete_call, args); intrinsic.has_value())
                {
                    this->emit(current_block, intrinsic.value());
                    co_await co_generate_builtin_return(current_block);
                    co_await co_generate_dtor_references();
                    co_return get_result();
                }
            }

            co_await co_generate_copy_ctor_delegates(current_block, func);

            co_await co_generate_builtin_return(current_block);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_move_ctor(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();
            block_index current_block = block_index(0);

            if (typeis< submember >(func.temploid.templexoid))
            {
                submember const& member = as< submember >(func.temploid.templexoid);
                if (co_await rpnx::querygraph::request< symbol_type_query >(member.of) == symbol_kind::interface_)
                {
                    if (!co_await this->co_try_emit_interface_builtin_from_locals(current_block, func))
                    {
                        throw compiler_bug("Interface move constructor routine is not implemented: " + quxlang::to_string(func));
                    }
                    co_await co_generate_builtin_return(current_block);
                    co_await co_generate_dtor_references();
                    co_return get_result();
                }
            }

            auto thisidx = this->local_value_direct_lookup(current_block, "THIS");
            auto otheridx = this->local_value_direct_lookup(current_block, "OTHER");
            if (thisidx.has_value() && otheridx.has_value())
            {
                codegen_invocation_args args;
                args.named["THIS"] = *thisidx;
                args.named["OTHER"] = *otheridx;
                instatype concrete_params = co_await rpnx::querygraph::request< instanciation_concrete_params_query >(func);
                invotype concrete_call = invotype_from_instatype(concrete_params);
                if (typeis< submember >(func.temploid.templexoid))
                {
                    submember const& member = as< submember >(func.temploid.templexoid);
                    symbol_kind const member_kind = co_await rpnx::querygraph::request< symbol_type_query >(member.of);
                    if (member_kind == symbol_kind::enum_ || member_kind == symbol_kind::flagset_)
                    {
                        this->emit(current_block, vmir2::load_from_ref{.from_reference = get_local_index(*otheridx), .to_value = get_local_index(*thisidx)});
                        co_await co_generate_builtin_return(current_block);
                        co_await co_generate_dtor_references();
                        co_return get_result();
                    }
                }
                if (auto intrinsic = this->intrinsic_instruction(func, concrete_call, args); intrinsic.has_value())
                {
                    this->emit(current_block, intrinsic.value());
                    co_await co_generate_builtin_return(current_block);
                    co_await co_generate_dtor_references();
                    co_return get_result();
                }
            }

            co_await co_generate_move_ctor_delegates(current_block, func);
            co_await co_generate_builtin_return(current_block);
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_assignment(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            co_await co_generate_arg_info(func);
            this->generate_entry_block();
            block_index current_block = block_index(0);

            if (typeis< submember >(func.temploid.templexoid))
            {
                submember const& member = as< submember >(func.temploid.templexoid);
                symbol_kind const member_kind = co_await rpnx::querygraph::request< symbol_type_query >(member.of);
                if (member_kind == symbol_kind::interface_)
                {
                    if (!co_await this->co_try_emit_interface_builtin_from_locals(current_block, func))
                    {
                        throw compiler_bug("Interface assignment routine is not implemented: " + quxlang::to_string(func));
                    }
                    co_await co_generate_builtin_return(current_block);
                    co_await co_generate_dtor_references();
                    co_return get_result();
                }
                if (member_kind == symbol_kind::enum_ || member_kind == symbol_kind::flagset_)
                {
                    auto thisidx = this->local_value_direct_lookup(current_block, "THIS");
                    auto otheridx = this->local_value_direct_lookup(current_block, "OTHER");
                    if (!thisidx.has_value() || !otheridx.has_value())
                    {
                        throw compiler_bug("Nominal integer assignment is missing THIS or OTHER");
                    }
                    this->emit(current_block, vmir2::store_to_ref{.from_value = get_local_index(*otheridx), .to_reference = get_local_index(*thisidx)});
                    co_await co_generate_builtin_return(current_block);
                    co_await co_generate_dtor_references();
                    co_return get_result();
                }
            }

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

        auto co_generate_builtin_dtor(instanciation_reference const& func) -> co_type< quxlang::vmir2::functanoid_routine3 >
        {

            co_await co_generate_arg_info(func);
            throw rpnx::unimplemented();
            co_await co_generate_dtors();
            co_await co_generate_dtor_references();
            co_return get_result();
        }

        auto co_generate_builtin_return(block_index bidx) -> co_type< void >
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
            result.static_snapshots = state.static_snapshots;

            return result;
        }

        auto co_generate_body(block_index& current_block, instanciation_reference const& func) -> co_type< void >
        {
            auto const& inst = func;

            auto& function_ref = inst.temploid;

            auto function_decl_opt = co_await rpnx::querygraph::request< function_declaration_query >(function_ref);
            assert(function_decl_opt.has_value());
            ast2_function_declaration& function_decl = function_decl_opt.value();

            co_await co_generate_function_block(current_block, function_decl.definition.body, "body");

            // TODO: Check if default return is allowed.
            this->generate_return(current_block);
            this->validate_no_pending_gotos();

            co_return;
        }

        void generate_return(block_index idx)
        {
            this->set_terminator(idx, vmir2::ret());
        }

        auto co_generate_dtor_references() -> co_type< void >
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
                auto dtor = co_await rpnx::querygraph::request< class_default_dtor_query >(slot.type);
                if (dtor)
                {
                    assert(!this->state.non_trivial_dtors.contains(slot.type) || this->state.non_trivial_dtors[slot.type] == *dtor);
                    this->state.non_trivial_dtors[slot.type] = *dtor;
                }
            }

            co_return;
        }

        auto co_return_value(block_index& current_block, value_index return_value) -> co_type< void >
        {
            auto return_arg_opt = this->local_value_direct_lookup(current_block, "RETURN");

            if (!return_arg_opt.has_value())
            {
                throw compiler_bug("RETURN parameter not found");
            }

            auto return_arg = return_arg_opt.value();

            codegen_invocation_args args;
            args.named["THIS"] = return_arg;
            args.named["OTHER"] = return_value;

            auto return_type = current_type(current_block, return_arg);
            if (!typeis< nvalue_slot >(return_type))
            {
                throw compiler_bug("RETURN parameter has the wrong type");
            }
            return_type = type_symbol(as< nvalue_slot >(return_type).target);
            auto ctor = submember{.of = return_type, .name = "CONSTRUCTOR"};
            co_await co_gen_call_functum(current_block, ctor, args);
            this->generate_return(current_block);
            co_return;
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_return_statement const& st) -> co_type< void >
        {
            auto return_arg_opt = this->local_value_direct_lookup(current_block, "RETURN");

            if (return_arg_opt.has_value())
            {
                auto return_arg = return_arg_opt.value();

                if (st.expr.has_value())
                {
                    auto expr_index = co_await co_generate_expr(current_block, st.expr.value());

                    co_await co_return_value(current_block, expr_index);
                    co_return;
                }

                co_await co_generate_builtin_return(current_block);
            }
            else if (st.expr.has_value() && this->state.declared_return_type.has_value() && is_template(*this->state.declared_return_type))
            {
                value_index expr_index = co_await co_generate_expr(current_block, st.expr.value());
                type_symbol expr_type = this->current_type(current_block, expr_index);
                type_symbol deduced_return_type = this->deduce_return_type_from_expression(*this->state.declared_return_type, expr_type);
                co_await co_publish_deduced_return_type(deduced_return_type);
                this->create_return_parameter(std::move(deduced_return_type));
                co_await co_return_value(current_block, expr_index);
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

                auto return_type = co_await rpnx::querygraph::request< functanoid_return_type_query >(ctx.get_as< instanciation_reference >());
                assert(typeis< void_type >(return_type));
                this->generate_return(current_block);
            }

            co_return;
        }

        [[nodiscard]] auto co_generate_fblock_statement(block_index& current_block, function_statement const& st) -> co_type< void >
        {
            auto location_scope = this->scoped_source_location(get_location(st));
            try
            {
                co_await rpnx::apply_visitor< co_type< void > >(st,
                                                                [&](auto st) -> co_type< void >
                                                                {
                                                                    co_return co_await this->co_generate_statement_ovl(current_block, st);
                                                                });
            }
            catch (compilation_error& err)
            {
                err.traceback.push_back(trace_frame{.trace_context = "statement", .location = get_location(st)});
                throw;
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

        [[nodiscard]] auto co_generate_function_block(block_index& current_block, function_block const& block, std::string block_from) -> co_type< void >
        {
            assert(!this->state.blocks.at(current_block).terminator.has_value());
            this->state.static_scopes.emplace_back();
            auto new_block = this->generate_subblock(current_block, block_from + "_block_new");

            assert(!this->state.blocks.at(new_block).terminator.has_value());

            auto after_block = this->generate_subblock(current_block, block_from + "_block_after");
            assert(!this->state.blocks.at(after_block).terminator.has_value());

            this->generate_jump(current_block, new_block);

            for (auto const& statement : block.statements)
            {
                if (this->state.blocks.at(new_block).terminator.has_value())
                {
                    new_block = this->generate_subblock(new_block, block_from + "_block_unreachable");
                }
                co_await co_generate_fblock_statement(new_block, statement);
            }

            this->generate_fallthrough_jump(new_block, after_block);

            assert(this->state.blocks.at(current_block).terminator.has_value());
            current_block = after_block;
            assert(!this->state.blocks.at(after_block).terminator.has_value());
            this->state.static_scopes.pop_back();
            co_return;
        }

        void generate_jump(block_index from, block_index to, std::string_view action = "jump")
        {
            auto& from_block = this->state.blocks.at(from);
            if (from_block.terminator.has_value())
            {
                throw compiler_bug("Cannot " + std::string(action) + " from a block that already has a terminator");
            }

            vmir2::jump jump_instruction{.target = to};
            this->set_terminator(from, jump_instruction);
        }

        void generate_fallthrough_jump(block_index from, block_index to)
        {
            if (this->state.blocks.at(from).terminator.has_value())
            {
                return;
            }
            this->generate_jump(from, to);
        }

        bool has_terminator(block_index const& block) const
        {
            return this->state.blocks.at(block).terminator.has_value();
        }

        auto co_apply_instantiation_scope(instanciation_reference const& inst) -> co_type< void >
        {
            auto inst_kind = co_await rpnx::querygraph::request< symbol_type_query >(inst.temploid);
            if (inst_kind != symbol_kind::template_)
            {
                co_return;
            }

            auto scoped_bindings = instantiation_scope_for(inst);
            for (auto& [name, definition] : scoped_bindings.scoped_definitions)
            {
                this->state.scoped_definitions[std::move(name)] = std::move(definition);
            }
            for (auto& [symbol, binding] : scoped_bindings.statics)
            {
                this->state.statics[std::move(symbol)] = codegen_static{
                    .type = std::move(binding.type),
                    .value = std::move(binding.value),
                    .mutation_result_id = std::nullopt,
                };
            }
        }

        auto co_apply_context_instantiation_scopes(type_symbol context) -> co_type< void >
        {
            std::optional< type_symbol > current_context = std::move(context);
            while (current_context.has_value())
            {
                if (typeis< instanciation_reference >(*current_context))
                {
                    co_await co_apply_instantiation_scope(as< instanciation_reference >(*current_context));
                }
                current_context = type_parent(*current_context);
            }
            co_return;
        }

        auto create_return_parameter(type_symbol return_type) -> value_index
        {
            if (this->state.params.named.contains("RETURN"))
            {
                throw compiler_bug("RETURN parameter is already defined");
            }

            type_symbol return_parameter_type = create_nslot(return_type);
            validate_codegen_type(return_type, "Return value type");
            validate_codegen_type(return_parameter_type, "Return parameter type");
            value_index return_valueidx = this->create_local_value(std::move(return_type));
            local_index return_local_index = get_local_index(return_valueidx);

            this->state.params.named["RETURN"] = {
                .type = std::move(return_parameter_type),
                .local_index = return_local_index,
            };
            this->state.top_level_lookups["RETURN"] = return_valueidx;

            vmir2::slot_state return_slot_state;
            return_slot_state.stage = vmir2::slot_stage::dead;
            return_slot_state.storage_valid = true;
            for (codegen_block& block : this->state.blocks)
            {
                block.entry_state[return_local_index] = return_slot_state;
                block.current_state[return_local_index] = return_slot_state;
            }

            return return_valueidx;
        }

        auto co_lookup_declared_return_type(instanciation_reference const& inst) -> co_type< type_symbol >
        {
            std::optional< builtin_function_info > primitive = co_await rpnx::querygraph::request< function_primitive_query >(inst.temploid);
            if (primitive.has_value())
            {
                type_symbol return_type = primitive->return_type;
                if (is_contextual(return_type) || is_template(return_type))
                {
                    contextual_type_reference lookup_input{.context = inst, .type = std::move(return_type)};
                    std::optional< type_symbol > lookup_result = co_await rpnx::querygraph::request< lookup_query >(lookup_input);
                    if (!lookup_result.has_value())
                    {
                        throw compiler_bug("Primitive function return type could not be resolved");
                    }
                    co_return lookup_result.value();
                }
                co_return return_type;
            }

            std::optional< ast2_function_declaration > declaration = co_await rpnx::querygraph::request< function_declaration_query >(inst.temploid);
            if (!declaration.has_value())
            {
                throw compiler_bug("No function declaration");
            }

            type_symbol declared_return_type = declaration->definition.return_type.value_or(type_symbol(void_type{}));
            contextual_type_reference lookup_input{.context = inst, .type = std::move(declared_return_type)};
            std::optional< type_symbol > lookup_result = co_await rpnx::querygraph::request< lookup_query >(lookup_input);
            if (!lookup_result.has_value())
            {
                throw compiler_bug("Function return type could not be resolved");
            }
            co_return lookup_result.value();
        }

        auto co_publish_deduced_return_type(type_symbol return_type) -> co_type< void >
        {
            this->state.deduced_return_type = return_type;

            if constexpr (rpnx::querygraph::query_handler_produced_subqueries_t< handler_spec >::template contains< functanoid_deduced_return_type >())
            {
                co_yield rpnx::querygraph::subquery_result< functanoid_deduced_return_type >(std::monostate{}, std::move(return_type));
            }
            else
            {
                (void)return_type;
            }

            co_return;
        }

        auto deduce_return_type_from_expression(type_symbol declared_return_type, type_symbol expression_type) -> type_symbol
        {
            std::optional< template_match_results > match = match_template(declared_return_type, expression_type);
            if (match.has_value())
            {
                return match->type;
            }

            throw semantic_compilation_error("Return expression type " + to_string(expression_type) + " does not match declared return template " + to_string(declared_return_type));
        }

        auto co_generate_arg_info(instanciation_reference func) -> co_type< void >
        {
            QUXLANG_DEBUG_VALUE(quxlang::to_string(func));
            // Precondition: Func is a fully instanciated symbol

            assert(!type_is_contextual(func));
            instanciation_reference inst = func;
            auto concrete_params = co_await rpnx::querygraph::request< instanciation_concrete_params_query >(inst);

            // This function should be called before generating any blocks.
            assert(this->state.blocks.empty());

            co_await co_apply_context_instantiation_scopes(inst);

            type_symbol return_type = co_await co_lookup_declared_return_type(inst);
            this->state.declared_return_type = return_type;

            if (!is_template(return_type) && !typeis< void_type >(return_type))
            {
                this->create_return_parameter(std::move(return_type));
            }

            auto create_parameter_lookup = [&](type_symbol const& param_type, auto publish_runtime_parameter) -> value_index
            {
                validate_codegen_type(param_type, "Routine parameter type");
                if (typeis< attached_type_reference >(param_type))
                {
                    attached_type_reference const& attached = as< attached_type_reference >(param_type);
                    if (typeis< void_type >(attached.carrying_type))
                    {
                        return this->create_binding(value_index(0), attached.attached_symbol);
                    }

                    type_symbol runtime_type = attached.carrying_type;
                    validate_codegen_type(runtime_type, "Attached runtime parameter carrier type");
                    value_index carrier_idx = this->create_local_value(parameter_local_type(runtime_type));
                    publish_runtime_parameter(std::move(runtime_type), carrier_idx);
                    return this->create_binding(carrier_idx, attached.attached_symbol);
                }

                type_symbol runtime_type = param_type;
                validate_codegen_type(runtime_type, "Runtime parameter type");
                value_index param_idx = this->create_local_value(parameter_local_type(runtime_type));
                publish_runtime_parameter(std::move(runtime_type), param_idx);
                return param_idx;
            };

            auto create_named_parameter_lookup = [&](std::string const& api_name, type_symbol const& param_type) -> value_index
            {
                return create_parameter_lookup(param_type,
                                               [&](type_symbol runtime_type, value_index param_idx)
                                               {
                                                   local_index param_local = get_local_index(param_idx);
                                                   this->state.params.named[api_name] = {
                                                       .type = std::move(runtime_type),
                                                       .local_index = param_local,
                                                   };
                                               });
            };

            auto create_positional_parameter_lookup = [&](type_symbol const& param_type) -> value_index
            {
                return create_parameter_lookup(param_type,
                                               [&](type_symbol runtime_type, value_index param_idx)
                                               {
                                                   this->state.params.positional.push_back(vmir2::routine_parameter{
                                                       .type = std::move(runtime_type),
                                                       .local_index = get_local_index(param_idx),
                                                   });
                                               });
            };

            auto register_parameter_lookup_name = [&](std::optional< std::string > const& local_name, std::string const& api_name, value_index arg_idx) -> void
            {
                if (local_name.has_value())
                {
                    this->state.top_level_lookups[local_name.value()] = arg_idx;
                    this->state.top_level_lookups_weak[api_name] = arg_idx;
                }
                else
                {
                    this->state.top_level_lookups[api_name] = arg_idx;
                }
            };

            auto declaration = co_await rpnx::querygraph::request< function_declaration_query >(inst.temploid);
            if (declaration.has_value())
            {
                std::size_t positional_index = 0;
                std::set< std::string > handled_named_parameters;
                for (auto const& param : declaration->header.call_parameters)
                {
                    if (param.api_name.has_value())
                    {
                        auto const& api_name = param.api_name.value();
                        handled_named_parameters.insert(api_name);
                        auto const& param_type = parameter_instantiation_type(concrete_params.named.at(api_name));
                        value_index arg_idx = create_named_parameter_lookup(api_name, param_type);
                        register_parameter_lookup_name(param.name, api_name, arg_idx);
                        continue;
                    }

                    if (!param.is_pack)
                    {
                        auto const& param_type = parameter_instantiation_type(concrete_params.positional.at(positional_index));
                        value_index param_idx = create_positional_parameter_lookup(param_type);
                        if (param.name.has_value())
                        {
                            this->state.top_level_lookups[param.name.value()] = param_idx;
                        }
                        positional_index++;
                        continue;
                    }

                    codegen_pack pack;
                    while (positional_index < concrete_params.positional.size())
                    {
                        auto const& param_type = parameter_instantiation_type(concrete_params.positional.at(positional_index));
                        value_index param_idx = create_positional_parameter_lookup(param_type);
                        pack.values.push_back(param_idx);
                        pack.types.push_back(param_type);
                        positional_index++;
                    }

                    if (param.name.has_value())
                    {
                        this->state.packs[param.name.value()] = std::move(pack);
                    }
                }

                for (auto const& [api_name, param] : concrete_params.named)
                {
                    if (api_name == "RETURN")
                    {
                        continue;
                    }
                    if (handled_named_parameters.contains(api_name))
                    {
                        continue;
                    }
                    auto const& param_type = parameter_instantiation_type(param);
                    value_index arg_idx = create_named_parameter_lookup(api_name, param_type);
                    this->state.top_level_lookups[api_name] = arg_idx;
                }

                if (positional_index != concrete_params.positional.size())
                {
                    throw compiler_bug("Function argument generation did not consume all positional arguments");
                }
                co_return;
            }

            auto arg_names = co_await rpnx::querygraph::request< function_param_names_query >(inst.temploid);
            auto formal_ensig = co_await rpnx::querygraph::request< temploid_formal_ensig_query >(inst.temploid);
            if (!formal_ensig.has_value())
            {
                throw compiler_bug("Formal ensig not found for function argument generation");
            }

            std::size_t positional_index = 0;
            for (std::size_t interface_index = 0; interface_index < formal_ensig->interface.positional.size(); interface_index++)
            {
                auto const& interface_param = formal_ensig->interface.positional.at(interface_index);
                if (interface_param.is_pack)
                {
                    while (positional_index < concrete_params.positional.size())
                    {
                        type_symbol const& param_type = parameter_instantiation_type(concrete_params.positional.at(positional_index));
                        create_positional_parameter_lookup(param_type);
                        positional_index++;
                    }
                    continue;
                }

                type_symbol const& param_type = parameter_instantiation_type(concrete_params.positional.at(positional_index));
                value_index param_idx = create_positional_parameter_lookup(param_type);
                if (interface_index < arg_names.positional.size() && arg_names.positional.at(interface_index).has_value())
                {
                    this->state.top_level_lookups[*arg_names.positional.at(interface_index)] = param_idx;
                }
                positional_index++;
            }
            if (positional_index != concrete_params.positional.size())
            {
                throw compiler_bug("Builtin function argument generation did not consume all positional arguments");
            }
            for (auto const& [api_name, param] : concrete_params.named)
            {
                if (api_name == "RETURN")
                {
                    continue;
                }
                auto const& param_type = parameter_instantiation_type(param);
                std::optional< std::string > arg_name;
                if (arg_names.named.contains(api_name))
                {
                    arg_name = arg_names.named.at(api_name);
                }
                value_index arg_idx = create_named_parameter_lookup(api_name, param_type);

                // If a local name is provided, it's strongly defined and the API name is weakly defined.
                // Otherwise, the API name is strongly defined.

                // Weakly defined names can be shadowed by strongly defined names.

                // TODO: Check for conflicts with existing names in the top-level lookups.

                register_parameter_lookup_name(arg_name, api_name, arg_idx);
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

        auto co_generate_lambda_constructor(block_index& current_block, instanciation_reference const& func, lambda_symbol_info const& lambda) -> co_type< void >
        {
            (void)func;
            std::vector< type_symbol > capture_types =
                co_await rpnx::querygraph::subquery_request< lambda_capture_set_subquery >(as< instanciation_reference >(lambda.parent_functanoid), lambda.index);

            std::optional< value_index > this_value = this->local_value_direct_lookup(current_block, "THIS");
            if (!this_value.has_value())
            {
                throw compiler_bug("Lambda constructor has no THIS parameter");
            }

            codegen_invocation_args fields_args;
            for (std::size_t i = 0; i < capture_types.size(); i++)
            {
                fields_args.named[lambda_capture_field_name(i)] = this->create_local_value(capture_types.at(i));
            }
            this->emit(current_block, vmir2::struct_init_start{.on_value = get_local_index(*this_value), .fields = get_invocation_args(fields_args)});

            for (std::size_t i = 0; i < capture_types.size(); i++)
            {
                std::string argument_name = "__CAPTURE_ARG" + std::to_string(i);
                std::optional< value_index > argument = this->local_value_direct_lookup(current_block, argument_name);
                if (!argument.has_value())
                {
                    throw compiler_bug("Lambda constructor missing positional capture argument: " + argument_name);
                }

                type_symbol const& field_type = capture_types.at(i);
                codegen_invocation_args field_ctor_args;
                field_ctor_args.named["THIS"] = fields_args.named.at(lambda_capture_field_name(i));
                field_ctor_args.named["OTHER"] = *argument;
                type_symbol field_constructor = submember{.of = field_type, .name = "CONSTRUCTOR"};
                co_await this->co_gen_call_functum(current_block, std::move(field_constructor), std::move(field_ctor_args), allowed_adaptations::source_rebinding);
            }

            co_return;
        }

        auto co_apply_lambda_operator_environment(block_index& current_block, instanciation_reference const& func) -> co_type< void >
        {
            auto lambda = parse_lambda_operator_symbol(func.temploid.templexoid);
            if (!lambda.has_value())
            {
                co_return;
            }

            auto env = co_await rpnx::querygraph::subquery_request< lambda_environment_subquery >(as< instanciation_reference >(lambda->parent_functanoid), lambda->index);
            this->state.scoped_definitions = std::move(env.scoped_definitions);
            this->state.statics.clear();
            for (auto& [symbol, input] : env.statics)
            {
                this->state.statics[std::move(symbol)] = codegen_static{
                    .type = std::move(input.type),
                    .value = std::move(input.value),
                    .mutation_result_id = std::nullopt,
                };
            }

            auto this_value = this->local_value_direct_lookup(current_block, "THIS");
            if (!this_value.has_value())
            {
                throw compiler_bug("Lambda operator has no THIS parameter");
            }
            for (auto const& [name, index] : env.capture_indices)
            {
                auto field_ref = co_await this->co_generate_dot_access(current_block, *this_value, lambda_capture_field_name(index));
                this->state.top_level_lookups[name] = field_ref;
            }
            co_return;
        }

        [[nodiscard]] auto co_generate_functanoid(instanciation_reference func) -> co_type< vmir2::functanoid_routine3 >
        {
            assert(!type_is_contextual(func));
            co_await this->co_generate_arg_info(func);
            this->generate_entry_block();
            block_index current_block(0);
            co_await this->co_apply_lambda_operator_environment(current_block, func);
            if (co_await rpnx::querygraph::request< function_builtin_query >(func.temploid) == builtin_function_kind::not_builtin)
            {
                if (std::optional< lambda_symbol_info > lambda_constructor = parse_lambda_constructor_symbol(func.temploid.templexoid); lambda_constructor.has_value())
                {
                    co_await this->co_generate_lambda_constructor(current_block, func, *lambda_constructor);
                    co_await co_generate_builtin_return(current_block);
                    co_await co_generate_dtors();
                    co_return get_result();
                }

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

        [[nodiscard]] auto co_generate_static_test(ast2_static_test const& test) -> co_type< vmir2::functanoid_routine3 >
        {
            this->generate_entry_block();
            block_index current_block(0);
            co_await co_generate_function_block(current_block, test.definition.body, "static_test_body");

            this->generate_return(current_block);

            this->validate_no_pending_gotos();
            co_await co_generate_dtors();

            co_return get_result();
        }

        auto add_nontrivial_default_dtor(type_symbol const& type, type_symbol const& dtor) -> void
        {
            state.non_trivial_dtors[type] = dtor;
        }

        auto co_generate_dtors() -> co_type< void >
        {
            // Loop through all local slots and check if they have non-trivial dtors, then add
            // dtor references to non_trivial_dtors if they do.
            for (auto const& slot : this->state.locals)
            {
                if (typeis< storage >(slot.type) || typeis< aligned_storage >(slot.type))
                {
                    continue;
                }
                auto dtor = co_await rpnx::querygraph::request< class_default_dtor_query >(slot.type);
                if (dtor.has_value())
                {
                    assert(!this->state.non_trivial_dtors.contains(slot.type) || this->state.non_trivial_dtors[slot.type] == *dtor);
                    this->state.non_trivial_dtors[slot.type] = *dtor;
                }
            }

            co_return;
        }

        [[nodiscard]] auto co_generate_struct_ctor_delegates(block_index& bidx, instanciation_reference const& func) -> co_type< void >
        {
            temploid_reference const& function = func.temploid;

            std::optional< ast2_function_declaration > const& function_ast = co_await rpnx::querygraph::request< function_declaration_query >(function);

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

        auto get_invocation_args(instatype const& concrete_params, codegen_invocation_args const& args) -> vmir2::invocation_args
        {
            vmir2::invocation_args result;
            for (auto const& [name, value] : args.named)
            {
                if (name == "RETURN")
                {
                    result.named[name] = get_local_index(value);
                    continue;
                }

                type_symbol param_type = parameter_instantiation_type(concrete_params.named.at(name));
                std::optional< type_symbol > runtime_type = runtime_type_for_attached_parameter(param_type);
                if (!runtime_type.has_value())
                {
                    continue;
                }

                result.named[name] = get_local_index(value);
            }

            for (std::size_t index = 0; index < args.positional.size(); index++)
            {
                type_symbol param_type = parameter_instantiation_type(concrete_params.positional.at(index));
                std::optional< type_symbol > runtime_type = runtime_type_for_attached_parameter(param_type);
                if (!runtime_type.has_value())
                {
                    continue;
                }

                result.positional.push_back(get_local_index(args.positional.at(index)));
            }
            return result;
        }

        auto co_copy_ref(block_index& current_block, value_index val) -> co_type< value_index >
        {
            type_symbol val_type = this->current_type(current_block, val);
            // This function should convert an mref to tref

            if (!val_type.type_is< ptrref_type >())
            {
                throw compiler_bug("Expected a reference type");
            }

            auto vptr = val_type.get_as< ptrref_type >();

            if (vptr.ptr_class != pointer_class::ref)
            {
                throw compiler_bug("Expected a reference type");
            }

            auto copy_idx = this->create_local_value(vptr);
            this->emit(current_block, vmir2::copy_reference{.from_index = get_local_index(val), .to_index = get_local_index(copy_idx)});
            co_return copy_idx;
        }

        auto co_generate_copy_ctor_delegates(block_index& current_block, instanciation_reference const& func) -> co_type< void >
        {
            instanciation_reference const& inst = func;
            auto const& sel = inst.temploid;

            auto functum = sel.templexoid;

            type_symbol cls;

            QUXLANG_COMPILER_BUG_IF(!typeis< submember >(functum), "Expected constructor to be submember");

            cls = as< submember >(functum).of;

            auto const& fields = co_await rpnx::querygraph::request< class_field_list_query >(cls);

            // Implicit copy ctor delegates just calls copy constructor on all fields.

            codegen_invocation_args fields_args;

            for (class_field const& fld : fields)
            {
                std::optional< type_symbol > field_storage_type = storage_type_for_attached_field(fld.type);
                if (!field_storage_type.has_value())
                {
                    continue;
                }
                auto fslot = this->create_local_value(*field_storage_type);
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
                std::optional< type_symbol > field_storage_type = storage_type_for_attached_field(fld.type);
                if (!field_storage_type.has_value())
                {
                    continue;
                }
                auto temporary_block = this->generate_subblock(current_block, "copy_ctor_temp_" + fld.name);
                auto after_ctor_block = this->generate_subblock(current_block, "copy_ctor_after_" + fld.name);
                this->generate_jump(current_block, temporary_block);
                auto other_idx_copy = co_await this->co_copy_ref(temporary_block, otheridx_value);
                auto other_field = co_await this->co_generate_dot_access(temporary_block, other_idx_copy, fld.name);
                if (typeis< attached_type_reference >(fld.type))
                {
                    other_field = this->attached_binding_carrier_value(temporary_block, other_field, fld.type);
                }
                auto field_type = *field_storage_type;
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

        auto co_generate_array_copy_ctor_delegates(block_index& current_block, instanciation_reference const& func) -> co_type< void >
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
                throw semantic_compilation_error("Array size is too large");
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

            type_symbol uintptr_type = co_await rpnx::querygraph::request< uintpointer_type_query >({});

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

        auto co_generate_swap_members(block_index& current_block, instanciation_reference const& func) -> co_type< void >
        {
            instanciation_reference const& inst = func;
            auto const& sel = inst.temploid;

            auto functum = sel.templexoid;

            type_symbol cls;

            QUXLANG_COMPILER_BUG_IF(!typeis< submember >(functum), "Expected constructor to be submember");

            cls = as< submember >(functum).of;

            auto const& fields = co_await rpnx::querygraph::request< class_field_list_query >(cls);

            for (class_field const& fld : fields)
            {
                std::optional< type_symbol > field_storage_type = storage_type_for_attached_field(fld.type);
                if (!field_storage_type.has_value())
                {
                    continue;
                }
                auto temp_block = this->generate_subblock(current_block, "swap_member_temp_" + fld.name);
                auto after_block = this->generate_subblock(current_block, "swap_member_after_" + fld.name);
                this->generate_jump(current_block, temp_block);
                current_block = temp_block;
                auto thisval = (co_await this->co_lookup_symbol(current_block, freebound_identifier{"THIS"})).value();
                auto otherval = (co_await this->co_lookup_symbol(current_block, freebound_identifier{"OTHER"})).value();
                auto this_field = co_await this->co_generate_dot_access(current_block, thisval, fld.name);
                auto other_field = co_await this->co_generate_dot_access(current_block, otherval, fld.name);
                if (typeis< attached_type_reference >(fld.type))
                {
                    this_field = this->attached_binding_carrier_value(current_block, this_field, fld.type);
                    other_field = this->attached_binding_carrier_value(current_block, other_field, fld.type);
                }
                auto field_type = *field_storage_type;
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

        auto co_generate_move(block_index& current_block, value_index val) -> co_type< value_index >
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
                this->emit(current_block, vmir2::cast_ptrref{.source_index = this->get_local_index(val), .target_index = this->get_local_index(tref)});
                co_return tref;
            }

            // TODO: Maybe this should be an error if e.g. it's a const ref?

            co_return val;
        }

        auto co_generate_move_ctor_delegates(block_index& current_block, instanciation_reference const& func) -> co_type< void >
        {

            instanciation_reference const& inst = func;
            auto const& sel = inst.temploid;

            auto functum = sel.templexoid;

            type_symbol cls;

            QUXLANG_COMPILER_BUG_IF(!typeis< submember >(functum), "Expected constructor to be submember");

            cls = as< submember >(functum).of;

            // This function is for default ctors, it should just default construct all member variables.

            auto const& fields = co_await rpnx::querygraph::request< class_field_list_query >(cls);

            codegen_invocation_args fields_args;

            for (class_field const& fld : fields)
            {
                std::optional< type_symbol > field_storage_type = storage_type_for_attached_field(fld.type);
                if (!field_storage_type.has_value())
                {
                    continue;
                }
                auto fslot = this->create_local_value(*field_storage_type);
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
                std::optional< type_symbol > field_storage_type = storage_type_for_attached_field(fld.type);
                if (!field_storage_type.has_value())
                {
                    continue;
                }
                auto temporary_block = this->generate_subblock(current_block, "move_ctor_temp_" + fld.name);
                auto after_ctor_block = this->generate_subblock(current_block, "move_ctor_after_" + fld.name);
                this->generate_jump(current_block, temporary_block);
                auto other_idx_copy = co_await this->co_copy_ref(temporary_block, otheridx_value);
                auto other_field = co_await this->co_generate_dot_access(temporary_block, other_idx_copy, fld.name);
                if (typeis< attached_type_reference >(fld.type))
                {
                    other_field = this->attached_binding_carrier_value(temporary_block, other_field, fld.type);
                }
                auto field_type = *field_storage_type;
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

        [[nodiscard]] auto co_generate_struct_ctor_delegates(block_index& current_block, instanciation_reference const& func, std::vector< delegate > delegates) -> co_type< void >
        {

            instanciation_reference const& inst = func;
            auto const& sel = inst.temploid;

            auto functum = sel.templexoid;

            type_symbol cls;

            QUXLANG_COMPILER_BUG_IF(!typeis< submember >(functum), "Expected constructor to be submember");

            cls = as< submember >(functum).of;

            // This function is for default ctors, it should just default construct all member variables.

            auto const& fields = co_await rpnx::querygraph::request< class_field_list_query >(cls);

            codegen_invocation_args fields_args;

            for (class_field const& fld : fields)
            {
                std::optional< type_symbol > field_storage_type = storage_type_for_attached_field(fld.type);
                if (!field_storage_type.has_value())
                {
                    continue;
                }
                auto fslot = this->create_local_value(*field_storage_type);
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

            for (delegate const& dlg : delegates)
            {
                auto field_it = std::find_if(fields.begin(), fields.end(), [&](class_field const& fld)
                                             { return fld.name == dlg.name; });
                if (field_it == fields.end())
                {
                    throw semantic_compilation_error("Constructor delegate names unknown field: " + dlg.name);
                }

                class_field const& fld = *field_it;
                std::optional< type_symbol > field_storage_type = storage_type_for_attached_field(fld.type);
                if (!field_storage_type.has_value())
                {
                    if (dlg.args.size() > 1)
                    {
                        throw semantic_compilation_error("Free attached binding delegate accepts at most one initializer");
                    }
                    if (dlg.args.size() == 1)
                    {
                        value_index init_idx = co_await co_generate_expr(current_block, dlg.args.front().value);
                        type_symbol init_type = this->current_type(current_block, init_idx);
                        if (init_type != fld.type)
                        {
                            throw semantic_compilation_error("Free attached binding delegate expected " + to_string(fld.type) + ", got " + to_string(init_type));
                        }
                    }
                    continue;
                }

                codegen_invocation_args args;
                args.named["THIS"] = fields_args.named.at(fld.name);

                if (typeis< attached_type_reference >(fld.type))
                {
                    if (dlg.args.size() != 1 || dlg.args.front().name.has_value())
                    {
                        throw semantic_compilation_error("Attached binding field delegates require exactly one positional initializer");
                    }
                    value_index init_idx = co_await co_generate_expr(current_block, dlg.args.front().value);
                    args.named["OTHER"] = this->attached_binding_carrier_value(current_block, init_idx, fld.type);
                }
                else
                {
                    for (expression_arg const& arg : dlg.args)
                    {
                        auto init_idx = co_await co_generate_expr(current_block, arg.value);
                        if (arg.name.has_value())
                        {
                            args.named[*arg.name] = init_idx;
                        }
                        else
                        {
                            args.positional.push_back(init_idx);
                        }
                    }
                }

                auto ctor = submember{.of = *field_storage_type, .name = "CONSTRUCTOR"};
                co_await this->co_gen_call_functum(current_block, ctor, args);
            }

            // TODO: Drop temporaries between loop iterations
            for (class_field const& fld : fields)
            {
                if (!found_delegate_names.contains(fld.name))
                {
                    std::optional< type_symbol > field_storage_type = storage_type_for_attached_field(fld.type);
                    if (!field_storage_type.has_value())
                    {
                        continue;
                    }
                    auto ctor = submember{.of = *field_storage_type, .name = "CONSTRUCTOR"};
                    codegen_invocation_args args;
                    args.named["THIS"] = fields_args.named.at(fld.name);

                    co_await this->co_gen_call_functum(current_block, ctor, args);
                }
            }

            // TODO: Should this execute before the body?
            // this->emit(current_block, vmir2::struct_complete_new{.on_value = get_local_index(thisidx_value)});

            // frame.block(current_block).emit(vmir2::struct_complete_new{.on_value = thisidx_value});
        }

        [[nodiscard]] auto co_generate_array_ctor_delegates(block_index& current_block, instanciation_reference const& func, std::vector< delegate > delegates) -> co_type< void >
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
                throw semantic_compilation_error("Array size is too large");
            }

            array_initializer_type init_type;
            init_type.count = res;
            init_type.element_type = element_type;

            auto thisidx = this->local_value_direct_lookup(current_block, "THIS");

            QUXLANG_COMPILER_BUG_IF(!thisidx.has_value(), "Expected THIS to be defined");

            auto thisidx_value = thisidx.value();

            auto initiailizer = create_local_value(init_type);

            this->emit(current_block, vmir2::array_init_start{.on_value = get_local_index(thisidx_value), .initializer = get_local_index(initiailizer)});

            if (!func.params.positional.empty())
            {
                if (func.params.positional.size() != static_cast< std::size_t >(res))
                {
                    throw semantic_compilation_error("Array positional constructor argument count does not match array length");
                }

                auto constructor = submember{.of = element_type, .name = "CONSTRUCTOR"};
                for (std::size_t i = 0; i < func.params.positional.size(); i++)
                {
                    auto element = this->create_local_value(element_type);
                    this->emit(current_block, vmir2::array_init_element{.initializer = get_local_index(initiailizer), .target = get_local_index(element)});

                    auto const& parameter = this->state.params.positional.at(i);
                    auto argument = this->get_value_index(parameter.local_index);
                    codegen_invocation_args args;
                    args.named["THIS"] = element;
                    args.named["OTHER"] = argument;
                    co_await this->co_gen_call_functum(current_block, constructor, args);
                }

                this->emit(current_block, vmir2::array_init_finish{.initializer = get_local_index(initiailizer)});
                co_return;
            }

            auto init_loop_condition_block = this->generate_subblock(current_block, "array_copy_condition_ctor_loop");
            auto init_loop_block = this->generate_subblock(current_block, "array_copy_ctor_loop");
            auto init_loop_done = this->generate_subblock(current_block, "array_copy_ctor_loop_done");

            this->generate_jump(current_block, init_loop_condition_block);

            type_symbol uintptr_type = co_await rpnx::querygraph::request< uintpointer_type_query >({});

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

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_assert_statement const& asrt) -> co_type< void >
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

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_place_statement const& st) -> co_type< void >
        {
            auto storage_ref = co_await co_generate_expr(current_block, st.at);
            auto storage_ref_type = this->current_type(current_block, storage_ref);
            auto storage_type = remove_ref(storage_ref_type);

            if (!is_ref(storage_ref_type) || (!typeis< storage >(storage_type) && !typeis< aligned_storage >(storage_type)))
            {
                throw semantic_compilation_error("invalid place on non-storage reference");
            }

            auto target_type = co_await this->co_resolve_type_symbol(current_block, st.type);
            auto ignored = co_await co_generate_place_expression_impl(current_block, storage_ref, target_type, st.assign_init, st.args);
            (void)ignored;
            co_return;
        }

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_destroy_statement const& st) -> co_type< void >
        {
            auto storage_ref = co_await co_generate_expr(current_block, st.at);
            auto storage_ref_type = this->current_type(current_block, storage_ref);
            auto storage_type = remove_ref(storage_ref_type);

            if (!is_ref(storage_ref_type) || (!typeis< storage >(storage_type) && !typeis< aligned_storage >(storage_type)))
            {
                throw semantic_compilation_error("invalid destroy on non-storage reference");
            }

            auto target_type = co_await this->co_resolve_type_symbol(current_block, st.type);
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

        [[nodiscard]] auto co_generate_statement_ovl(block_index& current_block, function_runtime_statement const& st) -> co_type< void >
        {
            block_index after_block = this->generate_subblock(current_block, "runtime_statement_after");
            block_index condition_block = this->generate_subblock(current_block, "runtime_statement_condition");
            block_index then_block = this->generate_subblock(current_block, "runtime_then");

            this->generate_jump(current_block, condition_block);

            if (!st.else_block.has_value())
            {
                if (st.condition == runtime_condition::CONSTEXPR)
                {
                    this->generate_runtime_constexpr(condition_block, then_block, after_block);
                }
                else
                {
                    this->generate_runtime_constexpr(condition_block, after_block, then_block);
                }

                // Then
                co_await co_generate_function_block(then_block, st.then_block, "runtime_then");
                this->generate_jump(then_block, after_block);
            }
            else
            {
                block_index else_block = this->generate_subblock(current_block, "runtime_else");
                if (st.condition == runtime_condition::CONSTEXPR)
                {
                    this->generate_runtime_constexpr(condition_block, then_block, else_block);
                }
                else
                {
                    this->generate_runtime_constexpr(condition_block, else_block, then_block);
                }

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

#endif // QUXLANG_CO_VMIR_GENERATOR2_HEADER_GUARD
