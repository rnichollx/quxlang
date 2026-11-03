// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_STATE_ENGINE_HEADER_GUARD
#define QUXLANG_VMIR2_STATE_ENGINE_HEADER_GUARD

#include <cstdint>
#include <map>
#include <set>
#include <rpnx/macros.hpp>
#include <optional>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/vmir2/vmir2.hpp>
#include <quxlang/vmir2/lifetime_transition_graph.hpp>
#include <vector>

namespace quxlang::vmir2
{

    using slot_vec = std::vector< vm_slot >;
    using state_diff = std::map< vmir2::local_index, std::pair< slot_state, slot_state > >;

    class codegen_state_engine
    {
      private:
        state_map& state;
        std::vector< vmir2::local_type > const& slot_info;
        vmir2::routine_parameters const& routine_params;
        /// Suppresses repeated whole-state validation while replaying validated instructions.
        bool validated_replay = false;

      public:
        /** Lists represented construction aliases in parent-before-child order. */
        static auto retained_storage_bindings(state_map const& current, local_index root) -> std::vector< local_index >
        {
            std::vector< local_index > pending{root};
            std::vector< local_index > bindings;
            for (std::size_t ordinal = 0; ordinal < pending.size(); ++ordinal)
            {
                local_index index = pending.at(ordinal);
                state_map::const_iterator represented = current.find(index);
                if (represented == current.end()) continue;
                bindings.push_back(index);
                slot_state const& binding = represented->second;
                if (binding.struct_delegates.has_value())
                {
                    for (struct_init_delegate const& delegate : *binding.struct_delegates) pending.push_back(delegate.value);
                }
                if (binding.array_delegates.has_value())
                {
                    for (local_index child : binding.array_delegates->positional) pending.push_back(child);
                    for (std::pair< std::string const, local_index > const& entry : binding.array_delegates->named) pending.push_back(entry.second);
                }
                pending.insert(pending.end(), binding.array_element_delegates.begin(), binding.array_element_delegates.end());
            }
            return bindings;
        }

        /** Verifies that an output contains a complete value or a complete set of construction delegates. */
        static bool initialization_completed(state_map const& states, local_index slot)
        {
            state_map::const_iterator value = states.find(slot);
            if (value == states.end() || !value->second.storage_valid)
            {
                return false;
            }
            if (value->second.stage == slot_stage::full)
            {
                return true;
            }
            if (value->second.stage != slot_stage::partial || !value->second.struct_delegates.has_value())
            {
                return false;
            }
            for (struct_init_delegate const& delegate : *value->second.struct_delegates)
            {
                if (!initialization_completed(states, delegate.value))
                {
                    return false;
                }
            }
            return true;
        }

        /**
         * Plans cleanup without mutating either state. Completed struct delegates are
         * destroyed before their owners, and each group uses reverse slot order.
         * Normal and exceptional exits preserve the destruction responsibility of
         * DESTROY parameters. Instruction transitions only end consumed lifetimes.
         * Consumers execute array-prefix destruction in reverse element order and
         * implement lifetime termination using their own storage representation.
         */
        static auto plan_transition(functanoid_routine3 const& routine, state_map const& current, state_map const& target,
                                    lifetime_transition_kind kind = lifetime_transition_kind::control_flow) -> std::vector< lifetime_action >
        {
            auto validate_initialization_parameter = [&](routine_parameter const& parameter)
            {
                if (!parameter.type.type_is< nvalue_slot >())
                {
                    return;
                }
                bool initialized = initialization_completed(current, parameter.local_index);
                if (!initialized)
                {
                    throw invalid_instruction_transition_error("VMIR normal return requires initialization of parameter slot " + std::to_string(parameter.local_index) + " of type " + to_string(parameter.type));
                }
            };
            if (kind == lifetime_transition_kind::normal_exit)
            {
                for (routine_parameter const& parameter : routine.parameters.positional)
                {
                    validate_initialization_parameter(parameter);
                }
                for (std::pair< std::string const, routine_parameter > const& parameter : routine.parameters.named)
                {
                    validate_initialization_parameter(parameter.second);
                }
            }
            lifetime_transition_graph graph(routine);
            return graph.flatten(graph.select(current, target, kind));
        }

        codegen_state_engine(state_map& state, std::vector< vmir2::local_type > const& slot_info, vmir2::routine_parameters const& params) : state(state), slot_info(slot_info), routine_params(params)
        {
        }

        void apply(vmir2::vm_instruction const& inst)
        {
            rpnx::apply_visitor< void >(inst,
                [&](auto const& x)
                {
                    check_state_valid();
                    apply_internal(x);
                    check_state_valid();
                });
        }

        /** Replays an instruction whose complete state invariants have already been validated. */
        void apply_validated(vmir2::vm_instruction const& instruction)
        {
            bool previous = validated_replay;
            validated_replay = true;
            try
            {
                rpnx::apply_visitor< void >(instruction, [&](auto& concrete) { apply_internal(concrete); });
            }
            catch (...)
            {
                validated_replay = previous;
                throw;
            }
            validated_replay = previous;
        }

        void apply_entry()
        {
            check_state_valid();
            for (local_index i = local_index(0); i < routine_params.positional.size(); i++)
            {
                auto const& param = routine_params.positional[i];
                local_index param_slot_index = param.local_index;

                if (param.type.template type_is< nvalue_slot >())
                {
                    state[param_slot_index].stage = slot_stage::dead;
                    state[param_slot_index].storage_valid = true;
                }
                else
                {
                    state[param_slot_index].stage = slot_stage::full;
                    state[param_slot_index].storage_valid = true;
                }
            }

            for (auto const& [name, param] : routine_params.named)
            {
                auto param_slot_index = param.local_index;

                if (param.type.template type_is< nvalue_slot >())
                {
                    state[param_slot_index].stage = slot_stage::dead;
                    state[param_slot_index].storage_valid = true;
                }
                else
                {
                    state[param_slot_index].stage = slot_stage::full;
                    state[param_slot_index].storage_valid = true;
                }
            }
            check_state_valid();
        }

        // apply_normal_exit is used to apply the normal exit state of a function,
        // when it returns without an exception.
        // Any values passed by value are destroyed, NVALUE[T] parameters are set to alive.
        void apply_normal_exit()
        {
            check_state_valid();
            for (local_index i = local_index(0); i < routine_params.positional.size(); i++)
            {
                auto const& param = routine_params.positional[i];
                local_index param_slot_index = param.local_index;

                if (param.type.template type_is< nvalue_slot >())
                {
                    // NVALUE[T] parameters are set to alive
                    state[param_slot_index].stage = slot_stage::full;
                    state[param_slot_index].struct_delegates = std::nullopt;
                    state[param_slot_index].array_delegates = std::nullopt;
                    state[param_slot_index].storage_valid = true;
                }
                else if (param.type.template type_is< dvalue_slot >())
                {
                    // DESTROY&& values are destroyed, but their storage remains valid upon exit
                    state[param_slot_index].stage = slot_stage::dead;
                    state[param_slot_index].storage_valid = true;
                    state[param_slot_index].struct_delegates = std::nullopt;
                    state[param_slot_index].array_delegates = std::nullopt;
                }
                else
                {
                    // other values no longer exist after the function returns
                    state[param_slot_index].stage = slot_stage::dead;
                    state[param_slot_index].storage_valid = false;
                    state[param_slot_index].struct_delegates = std::nullopt;
                    state[param_slot_index].array_delegates = std::nullopt;
                }
            }
            for (auto const& [name, param] : routine_params.named)
            {
                auto param_slot_index = param.local_index;

                if (param.type.template type_is< nvalue_slot >())
                {
                    // NVALUE[T] parameters are set to alive
                    state[param_slot_index].stage = slot_stage::full;
                    state[param_slot_index].struct_delegates = std::nullopt;
                    state[param_slot_index].array_delegates = std::nullopt;
                    state[param_slot_index].storage_valid = true;
                }
                else if (param.type.template type_is< dvalue_slot >())
                {
                    // DESTROY& values are destroyed, but their storage remains valid upon exit
                    state[param_slot_index].stage = slot_stage::dead;
                    state[param_slot_index].storage_valid = true;
                    state[param_slot_index].struct_delegates = std::nullopt;
                    state[param_slot_index].array_delegates = std::nullopt;
                }
                else
                {
                    // other values are destroyed
                    state[param_slot_index].stage = slot_stage::dead;
                    state[param_slot_index].storage_valid = false;
                    state[param_slot_index].struct_delegates = std::nullopt;
                    state[param_slot_index].array_delegates = std::nullopt;
                }
            }
            check_state_valid();
        }

        void apply_exception_exit()
        {
            check_state_valid();
            for (local_index i = local_index(0); i < routine_params.positional.size(); i++)
            {
                auto const& param = routine_params.positional[i];
                local_index param_slot_index = param.local_index;

                if (param.type.template type_is< nvalue_slot >())
                {
                    // NVALUE[T] parameters are not set to alive
                    state[param_slot_index].stage = slot_stage::dead;

                    state[param_slot_index].struct_delegates = std::nullopt;
                    state[param_slot_index].array_delegates = std::nullopt;
                    state[param_slot_index].storage_valid = true;
                }
                else if (param.type.template type_is< dvalue_slot >())
                {
                    // DESTROY& values are destroyed, but their storage remains valid upon exit
                    state[param_slot_index].stage = slot_stage::dead;
                    state[param_slot_index].storage_valid = true;
                }
                else
                {
                    state[param_slot_index].stage = slot_stage::dead;
                    state[param_slot_index].storage_valid = false;
                }
            }
            for (auto const& [name, param] : routine_params.named)
            {
                auto param_slot_index = param.local_index;

                if (param.type.template type_is< nvalue_slot >())
                {
                    // NVALUE[T] parameters are *not* initialized when an exception is thrown,
                    // but their storage remains valid
                    state[param_slot_index].stage = slot_stage::dead;
                    state[param_slot_index].storage_valid = true;
                    state[param_slot_index].struct_delegates = std::nullopt;
                    state[param_slot_index].array_delegates = std::nullopt;
                }
                else if (param.type.template type_is< dvalue_slot >())
                {
                    // DESTROY&& values are destroyed, but their storage remains valid upon exit
                    state[param_slot_index].stage = slot_stage::dead;
                    state[param_slot_index].storage_valid = true;
                    state[param_slot_index].struct_delegates = std::nullopt;
                    state[param_slot_index].array_delegates = std::nullopt;
                }
                else
                {
                    state[param_slot_index].stage = slot_stage::dead;
                    state[param_slot_index].storage_valid = false;
                }
            }
            check_state_valid();
        }

        void check_state_valid()
        {
#ifndef NDEBUG
            if (validated_replay) return;
            for (state_map::value_type const& entry : state)
            {
                assert(entry.second.valid());
            }
#endif
        }

      private:
        void apply_internal(vmir2::assert_instr const& asrt)
        {
            consume(asrt.condition);
        }

        /** Consumes the Boolean condition required by an assumption. */
        void apply_internal(vmir2::assume const& instruction)
        {
            consume(instruction.condition);
        }

        /** Publishes active ownership after transferring omitted objects to retained storage. */
        void apply_internal(vmir2::lifetime_transfer const& instruction)
        {
            state = instruction.state;
        }

        /** Reattaches one existing retained object under its selected logical ownership. */
        void apply_internal(vmir2::lifetime_import const& instruction)
        {
            if (!instruction.state.valid())
                throw invalid_instruction_transition_error("lifetime import requires a valid ownership state");
            if (!instruction.state.storage_valid) state.erase(instruction.slot);
            else state[instruction.slot] = instruction.state;
        }

        /** Code alignment has no lifetime or value effects. */
        void apply_internal(vmir2::code_align const&) {}

        void apply_internal(vmir2::unimplemented const&)
        {
           // intentionally left empty, this is a no-op
        }

        void apply_internal(vmir2::lowering_error const&)
        {
            // A lowering error does not alter VMIR slot state.
        }

        void apply_internal(vmir2::swap const& swp)
        {
            consume(swp.a);
            consume(swp.b);
        }

        void apply_internal(vmir2::increment const& inc)
        {
            consume(inc.value);
            output(inc.result);
        }
        void apply_internal(vmir2::decrement const& dec)
        {
            consume(dec.value);
            output(dec.result);
        }
        void apply_internal(vmir2::preincrement const& pinc)
        {
            consume(pinc.target);
            output(pinc.target2);
        }
        void apply_internal(vmir2::predecrement const& pdec)
        {
            consume(pdec.target);
            output(pdec.target2);
        }
        void apply_internal(vmir2::to_bool const& tb)
        {
            consume(tb.from);
            output(tb.to);
        }
        void apply_internal(vmir2::iconv const& icv)
        {
            consume(icv.from);
            output(icv.to);
        }
        void apply_internal(vmir2::enum_cast const& instruction)
        {
            consume(instruction.integer);
            output(instruction.result);
        }
        void apply_internal(vmir2::enum_int_inrange const& instruction)
        {
            readonly(instruction.integer);
            output(instruction.result);
        }
        void apply_internal(vmir2::to_bool_not const& tbn)
        {
            consume(tbn.from);
            output(tbn.to);
        }
        void apply_internal(vmir2::load_const_zero const& lcz)
        {
            output(lcz.target);
        }

        void apply_internal(vmir2::load_const_bool const& lcb)
        {
            output(lcb.target);
        }
        void apply_internal(vmir2::load_type_index const& instruction)
        {
            output(instruction.result);
        }
        void apply_internal(vmir2::access_field const& acf)
        {
            consume(acf.base_index);
            output(acf.store_index);
        }
        void apply_internal(vmir2::access_array const& aca)
        {
            consume(aca.base_index);
            consume(aca.index_index);
            output(aca.store_index);
        }
        void apply_internal(vmir2::access_pointer const& acp)
        {
            consume(acp.base_index);
            consume(acp.index_index);
            output(acp.store_index);
        }
        void apply_internal(vmir2::invoke const& inv)
        {
            check_state_valid();
            auto ivk_func_inst = inv.what.get_as< instanciation_reference >();

            std::size_t runtime_positional_index = 0;
            for (std::size_t param_index = 0; param_index < ivk_func_inst.params.positional.size(); param_index++)
            {
                type_symbol arg_inst_type = parameter_instantiation_type(ivk_func_inst.params.positional.at(param_index));
                std::optional< type_symbol > runtime_type = parameter_runtime_type(arg_inst_type);
                if (!runtime_type.has_value())
                {
                    continue;
                }
                if (runtime_positional_index >= inv.args.positional.size())
                {
                    throw invalid_instruction_error("invoke missing positional runtime argument");
                }

                local_index arg_idx = inv.args.positional[runtime_positional_index];
                runtime_positional_index++;

                if (runtime_type->template type_is< nvalue_slot >())
                {
                    output(arg_idx);
                }
                else
                {
                    consume(arg_idx);
                }
            }
            if (runtime_positional_index != inv.args.positional.size())
            {
                throw invalid_instruction_error("invoke has extra positional runtime arguments");
            }

            for (auto const& [name, param] : ivk_func_inst.params.named)
            {
                type_symbol arg_inst_type = parameter_instantiation_type(param);
                std::optional< type_symbol > runtime_type = parameter_runtime_type(arg_inst_type);
                if (!runtime_type.has_value())
                {
                    continue;
                }

                auto arg_it = inv.args.named.find(name);
                if (arg_it == inv.args.named.end())
                {
                    throw invalid_instruction_error("invoke missing named runtime argument");
                }

                if (runtime_type->template type_is< nvalue_slot >())
                {
                    output(arg_it->second);
                }
                else
                {
                    consume(arg_it->second);
                }
            }
            if (auto return_it = inv.args.named.find("RETURN"); return_it != inv.args.named.end())
            {
                type_symbol return_type = nvalue_slot{.target = slot_info.at(return_it->second).type};
                if (return_type.template type_is< nvalue_slot >())
                {
                    output(return_it->second);
                }
                else
                {
                    consume(return_it->second);
                }
            }
            check_state_valid();
        }
        void apply_internal(vmir2::invoke_virtual const& inv)
        {
            check_state_valid();
            if (inv.args.positional.size() != inv.slot.signature.positional_parameters.size())
            {
                throw invalid_instruction_error("invoke_virtual positional argument count does not match its slot");
            }
            for (std::size_t index = 0; index < inv.args.positional.size(); ++index)
            {
                local_index const argument = inv.args.positional.at(index);
                if (inv.slot.signature.positional_parameters.at(index).type_is< nvalue_slot >())
                {
                    output(argument);
                }
                else
                {
                    consume(argument);
                }
            }
            std::map< std::string, local_index >::const_iterator const this_argument = inv.args.named.find("THIS");
            if (this_argument == inv.args.named.end())
            {
                throw invalid_instruction_error("invoke_virtual is missing THIS");
            }
            if (inv.slot.signature.this_parameter.type_is< nvalue_slot >())
            {
                output(this_argument->second);
            }
            else
            {
                consume(this_argument->second);
            }
            for (std::pair< std::string const, type_symbol > const& parameter : inv.slot.signature.named_parameters)
            {
                std::map< std::string, local_index >::const_iterator const argument = inv.args.named.find(parameter.first);
                if (argument == inv.args.named.end())
                {
                    throw invalid_instruction_error("invoke_virtual is missing named argument " + parameter.first);
                }
                if (parameter.second.type_is< nvalue_slot >())
                {
                    output(argument->second);
                }
                else
                {
                    consume(argument->second);
                }
            }
            for (std::pair< std::string const, local_index > const& argument : inv.args.named)
            {
                if (argument.first == "RETURN")
                {
                    output(argument.second);
                }
                else if (argument.first != "THIS" && !inv.slot.signature.named_parameters.contains(argument.first))
                {
                    throw invalid_instruction_error("invoke_virtual has extra named argument " + argument.first);
                }
            }
            check_state_valid();
        }
        void apply_internal(vmir2::interface_init const& init)
        {
            output(init.target);
        }
        void apply_internal(vmir2::interface_invoke const& inv)
        {
            check_state_valid();
            consume(inv.interface_value);

            for (std::size_t index = 0; index < inv.slot.concrete_params.positional.size(); index++)
            {
                local_index arg_idx = inv.args.positional.at(index);
                type_symbol arg_inst_type = inv.slot.concrete_params.positional.at(index);
                if (arg_inst_type.template type_is< nvalue_slot >())
                {
                    output(arg_idx);
                }
                else
                {
                    consume(arg_idx);
                }
            }

            for (std::pair< std::string const, local_index > const& named_arg : inv.args.named)
            {
                std::string const& name = named_arg.first;
                local_index index = named_arg.second;
                type_symbol arg_inst_type;
                if (name == "RETURN")
                {
                    arg_inst_type = nvalue_slot{.target = slot_info.at(index).type};
                }
                else
                {
                    arg_inst_type = inv.slot.concrete_params.named.at(name);
                }

                if (arg_inst_type.template type_is< nvalue_slot >())
                {
                    output(index);
                }
                else
                {
                    consume(index);
                }
            }
            check_state_valid();
        }
        void apply_internal(vmir2::interface_is_default const& is_default)
        {
            consume(is_default.interface_value);
            output(is_default.result);
        }
        void apply_internal(vmir2::invoke_indirect const& inv)
        {
            check_state_valid();
            consume(inv.what_index);

            auto callable_type = remove_ref(slot_info.at(inv.what_index).type);
            std::optional< procedure_type > proc_sig;

            if (typeis< procedure_type >(callable_type))
            {
                proc_sig = as< procedure_type >(callable_type);
            }
            else if (typeis< ptrref_type >(callable_type))
            {
                auto const& ptr = as< ptrref_type >(callable_type);
                if (ptr.ptr_class == pointer_class::instance && typeis< procedure_type >(ptr.target))
                {
                    proc_sig = as< procedure_type >(ptr.target);
                }
            }

            if (!proc_sig.has_value())
            {
                throw invalid_instruction_error("invoke_indirect requires a procedure reference or pointer");
            }

            for (std::size_t index = 0; index < inv.args.positional.size(); index++)
            {
                auto const arg_idx = inv.args.positional[index];
                auto const& arg_inst_type = proc_sig->signature.params.positional.at(index);
                if (arg_inst_type.template type_is< nvalue_slot >())
                {
                    output(arg_idx);
                }
                else
                {
                    consume(arg_idx);
                }
            }

            for (auto const& [name, index] : inv.args.named)
            {
                type_symbol arg_inst_type;
                if (name == "RETURN")
                {
                    arg_inst_type = nvalue_slot{.target = slot_info.at(index).type};
                }
                else
                {
                    arg_inst_type = proc_sig->signature.params.named.at(name);
                }

                if (arg_inst_type.template type_is< nvalue_slot >())
                {
                    output(index);
                }
                else
                {
                    consume(index);
                }
            }
            check_state_valid();
        }
        void apply_internal(vmir2::inheritance_cast const& instruction)
        {
            consume(instruction.source);
            output(instruction.result);
        }
        void apply_internal(vmir2::struct_dynamic_cast const& instruction)
        {
            consume(instruction.source);
            output(instruction.result);
        }
        void apply_internal(vmir2::struct_type_is const& instruction)
        {
            consume(instruction.source);
            output(instruction.result);
        }
        /** Consumes the queried pointer and initializes its dynamic type index. */
        void apply_internal(vmir2::struct_dynamic_type const& instruction)
        {
            consume(instruction.source);
            output(instruction.result);
        }
        void apply_internal(vmir2::struct_alloc_info const& instruction)
        {
            consume(instruction.source);
            output(instruction.storage_pointer);
            output(instruction.size);
            output(instruction.align);
        }
        void apply_internal(vmir2::get_procedure_ptr const& gpp)
        {
            output(gpp.pointer_index);
        }
        void apply_internal(vmir2::storage_init const& sin)
        {
            output(sin.storage);
        }
        /** A logical continuation checkpoint preserves the current object lifetimes. */
        void apply_internal(vmir2::co_save const&)
        {
        }
        /** Publishes a borrowed pointer to compiler-owned cleanup state. */
        void apply_internal(vmir2::co_cleanup_storage const& instruction)
        {
            output(instruction.result);
        }
        /** Changes cleanup progress without accessing the associated logical object. */
        void apply_internal(vmir2::co_cleanup_mark const& instruction)
        {
            if (instruction.status > 2) throw invalid_instruction_transition_error("Invalid coroutine cleanup retirement status");
        }
        void apply_internal(vmir2::storage_init_start const& sis)
        {
            readonly(sis.on_storage);
            if (state[sis.target_value].alive()) [[unlikely]]
            {
                throw invalid_instruction_transition_error("storage init delegate already alive");
            }
            state[sis.target_value].destroy_delegate = false;
            state[sis.target_value].is_projection = !sis.owns_lifetime;
            state[sis.target_value].retained_storage = sis.persistent;
            state[sis.target_value].storage_origin = sis.on_storage;
            state[sis.target_value].persistent_storage = sis.persistent && !sis.owns_lifetime;
            state[sis.target_value].storage_valid = true;
            state[sis.target_value].stage = slot_stage::dead;
        }
        /** Restores initialization metadata for one already bound retained object. */
        void restore_storage_delegates(storage_delegate_rebind const& instruction, bool persistent, bool retained)
        {
            if (instruction.struct_delegates.has_value() && instruction.struct_delegates->size() != instruction.struct_delegate_stages.size())
            {
                throw invalid_instruction_transition_error("storage rebind delegate stages do not match delegates");
            }
            slot_state& target = state[instruction.target_value];
            if (instruction.array_delegate_of_initializer.has_value())
            {
                readonly(*instruction.array_delegate_of_initializer);
                target.storage_valid = true;
                target.array_delegate_of_initializer = instruction.array_delegate_of_initializer;
                state[*instruction.array_delegate_of_initializer].array_element_delegates.insert(instruction.target_value);
            }
            if (!target.storage_valid)
            {
                throw invalid_instruction_transition_error("storage rebind delegate has no enclosing object binding");
            }
            target.stage = instruction.stage;
            target.owns_subobject_lifetime = instruction.owns_subobject_lifetime;
            target.persistent_storage = persistent;
            target.retained_storage = retained;
            target.nontrivial_dtor = instruction.nontrivial_dtor;
            target.lifecycle_phase = instruction.lifecycle_phase;
            target.destruction_started = instruction.destruction_started;
            target.construction_pending = instruction.construction_pending;
            target.struct_delegates = instruction.struct_delegates;
            target.array_delegates = instruction.array_delegates;
            if (instruction.struct_delegates.has_value())
            {
                for (std::size_t ordinal = 0; ordinal < instruction.struct_delegates->size(); ++ordinal)
                {
                    struct_init_delegate const& delegate = instruction.struct_delegates->at(ordinal);
                    if (state[delegate.value].alive())
                    {
                        throw invalid_instruction_transition_error("storage rebind struct delegate already alive");
                    }
                    slot_state& binding = state[delegate.value];
                    binding = slot_state{};
                    binding.stage = instruction.struct_delegate_stages.at(ordinal);
                    binding.storage_valid = true;
                    binding.delegate_of = instruction.target_value;
                    binding.struct_delegate_selector = delegate.selector;
                    binding.persistent_storage = persistent;
                    binding.retained_storage = retained;
                }
            }
            if (instruction.array_delegates.has_value())
            {
                local_index initializer = instruction.array_delegates->named.at("INIT");
                if (state[initializer].stage != slot_stage::full || !state[initializer].storage_valid)
                {
                    throw invalid_instruction_transition_error("storage rebind requires an existing array initializer binding");
                }
                slot_state& binding = state[initializer];
                binding.delegate_of = instruction.target_value;
                binding.persistent_storage = persistent;
                binding.retained_storage = retained;
            }
        }
        /** Reattaches externally owned storage without transferring its lifetime to the activation. */
        void apply_internal(vmir2::storage_rebind const& instruction)
        {
            readonly(instruction.on_storage);
            if (state[instruction.target_value].alive())
            {
                throw invalid_instruction_transition_error("storage rebind target already alive");
            }
            slot_state& target = state[instruction.target_value];
            target = slot_state{};
            target.storage_valid = true;
            target.storage_origin = instruction.on_storage;
            target.destroy_delegate = instruction.destroy_delegate;
            target.is_projection = !instruction.destroy_delegate && !instruction.owns_lifetime;
            restore_storage_delegates(storage_delegate_rebind{
                .target_value = instruction.target_value,
                .stage = instruction.stage,
                .struct_delegates = instruction.struct_delegates,
                .struct_delegate_stages = instruction.struct_delegate_stages,
                .array_delegates = instruction.array_delegates,
                .nontrivial_dtor = instruction.nontrivial_dtor,
                .lifecycle_phase = instruction.lifecycle_phase,
                .destruction_started = instruction.destruction_started,
                .construction_pending = instruction.construction_pending,
            }, !instruction.destroy_delegate && !instruction.owns_lifetime, true);
            for (storage_delegate_rebind const& binding : instruction.nested_delegates)
            {
                restore_storage_delegates(binding, !instruction.destroy_delegate && !instruction.owns_lifetime, true);
            }
        }
        void apply_internal(vmir2::storage_deinit_start const& sds)
        {
            readonly(sds.on_storage);
            if (state[sds.target_value].alive()) [[unlikely]]
            {
                throw invalid_instruction_transition_error("storage deinit delegate already alive");
            }
            state[sds.target_value].destroy_delegate = true;
            state[sds.target_value].is_projection = false;
            state[sds.target_value].persistent_storage = false;
            state[sds.target_value].retained_storage = false;
            state[sds.target_value].storage_origin = sds.on_storage;
            state[sds.target_value].storage_valid = true;
            state[sds.target_value].stage = slot_stage::full;
        }
        void apply_internal(vmir2::storage_pun const& spn)
        {
            consume(spn.from_storage);
            output(spn.to_reference);
        }
        void apply_internal(vmir2::get_underyling_storage const& instruction)
        {
            consume(instruction.object_pointer);
            output(instruction.storage_pointer);
        }
        void apply_internal(vmir2::fusion_active_index const& instruction)
        {
            readonly(instruction.subject);
            output(instruction.result);
        }
        void apply_internal(vmir2::fusion_has_alternative const& instruction)
        {
            readonly(instruction.subject);
            output(instruction.result);
        }
        void apply_internal(vmir2::fusion_is_valueless const& instruction)
        {
            readonly(instruction.subject);
            output(instruction.result);
        }
        void apply_internal(vmir2::fusion_storage_ref const& instruction)
        {
            if (is_ref(slot_info.at(static_cast< std::uint64_t >(instruction.subject)).type))
            {
                readonly(instruction.subject);
            }
            else
            {
                slot_state const& subject_state = state[instruction.subject];
                if (subject_state.stage != slot_stage::dead || !subject_state.storage_valid) [[unlikely]]
                {
                    throw invalid_instruction_transition_error("fusion storage projection requires a live reference or dead initialized output storage");
                }
            }
            output(instruction.result);
        }
        void apply_internal(vmir2::fusion_set_active const& instruction)
        {
            if (instruction.payload_storage.has_value())
            {
                consume(*instruction.payload_storage);
            }
            publish_fusion_state(instruction.target);
        }
        void apply_internal(vmir2::fusion_set_valueless const& instruction)
        {
            publish_fusion_state(instruction.target);
        }
        void apply_internal(vmir2::fusion_swap_boxed_state const& instruction)
        {
            readonly(instruction.a);
            readonly(instruction.b);
        }
        void apply_internal(vmir2::constexpr_alloc const& cal)
        {
            output(cal.result);
        }
        void apply_internal(vmir2::constexpr_alloc_multiple const& cal)
        {
            consume(cal.count);
            output(cal.result);
        }
        void apply_internal(vmir2::constexpr_dealloc const& cdl)
        {
            consume(cdl.pointer);
        }
        void apply_internal(vmir2::constexpr_dealloc_multiple const& cdl)
        {
            consume(cdl.pointer);
            consume(cdl.count);
        }
        /** Records the pointer produced by a managed JVM storage allocation. */
        void apply_internal(vmir2::jvm_allocate_object_storage const& allocation)
        {
            output(allocation.result);
        }
        /** Records the count consumed and pointer produced by a managed JVM sequence allocation. */
        void apply_internal(vmir2::jvm_allocate_multiple_object_storage const& allocation)
        {
            consume(allocation.count);
            output(allocation.result);
        }
        /** Consumes the pointer passed to a managed JVM storage deallocation. */
        void apply_internal(vmir2::jvm_deallocate_object_storage const& deallocation)
        {
            consume(deallocation.pointer);
        }
        /** Consumes the pointer and count passed to a managed JVM sequence deallocation. */
        void apply_internal(vmir2::jvm_deallocate_multiple_object_storage const& deallocation)
        {
            consume(deallocation.pointer);
            consume(deallocation.count);
        }
        void apply_internal(vmir2::jvm_gc_pointer_checked_cast const& conversion)
        {
            consume(conversion.source);
            output(conversion.result);
        }
        void apply_internal(vmir2::get_object_ref const& gor)
        {
            output(gor.target_ref);
        }
        void apply_internal(vmir2::get_antestatal_ref const& gar)
        {
            output(gar.target_ref);
        }
        void apply_internal(vmir2::initguard_global_get_ref const& igr)
        {
            output(igr.target_ref);
        }
        void apply_internal(vmir2::initguard_complete const& igr)
        {
            consume(igr.lock);
        }
        void apply_internal(vmir2::initguard_abort const& iga)
        {
            consume(iga.lock);
        }
        void apply_internal(vmir2::thread_destructor_register const& registration)
        {
            (void)registration;
        }
        void apply_internal(vmir2::make_reference const& mrf)
        {
            if (mrf.storage_only)
            {
                state[mrf.value_index].storage_valid = true;
                state[mrf.value_index].construction_pending = true;
            }
            else
            {
                readonly(mrf.value_index);
            }
            output(mrf.reference_index);
        }
        void apply_internal(vmir2::cast_ptrref const& crf)
        {
            consume(crf.source_index);
            output(crf.target_index);
        }
        /** Tracks operands crossing an external-memory interface. */
        void apply_internal(vmir2::ibc_load const& instruction)
        {
            consume(instruction.address);
            output(instruction.result);
        }
        /** Tracks operands crossing an external-memory interface. */
        void apply_internal(vmir2::ibc_write const& instruction)
        {
            consume(instruction.address);
            consume(instruction.value);
        }
        /** Tracks operands crossing an external-memory interface. */
        void apply_internal(vmir2::ibc_pun const& instruction)
        {
            consume(instruction.address);
            output(instruction.result);
        }
        /** Tracks operands crossing an external-memory interface. */
        void apply_internal(vmir2::ibc_getaddr const& instruction)
        {
            consume(instruction.pointer);
            output(instruction.result);
        }
        void apply_internal(vmir2::address_launder const& launder)
        {
            consume(launder.source_index);
            output(launder.target_index);
        }
        void apply_internal(vmir2::cast_constant const& cc)
        {
            consume(cc.source_index);
            output(cc.target_index);
        }
        void apply_internal(vmir2::constexpr_set_result const& csr)
        {
            consume(csr.target);
        }
        void apply_internal(vmir2::constexpr_set_result2 const& csr)
        {
            consume(csr.target);
        }
        void apply_internal(vmir2::constexpr_make_proxy const& cmp)
        {
            output(cmp.target);
        }
        void apply_internal(vmir2::constexpr_output_byte const& cob)
        {
            consume(cob.proxy);
            consume(cob.value);
        }
        void apply_internal(vmir2::load_const_value const& lcv)
        {
            output(lcv.target);
        }
        /** Consumes the square-root operand and initializes its result. */
        void apply_internal(vmir2::float_sqrt const& instruction)
        {
            consume(instruction.source);
            output(instruction.result);
        }
        void apply_internal(vmir2::canonicalize_float const& cpf)
        {
            consume(cpf.source);
            output(cpf.result);
        }
        void apply_internal(vmir2::get_value_byte const& gvb)
        {
            consume(gvb.source_reference);
            output(gvb.result);
        }
        void apply_internal(vmir2::set_value_byte const& svb)
        {
            consume(svb.target_reference);
            consume(svb.value);
        }
        void apply_internal(vmir2::make_pointer_to const& mpt)
        {
            readonly(mpt.of_index);
            output(mpt.pointer_index);
        }
        void apply_internal(vmir2::dereference_pointer const& drp)
        {
            consume(drp.from_pointer);
            output(drp.to_reference);
        }
        /** Consumes the relocated value and initializes its destination. */
        void apply_internal(vmir2::relocate_value const& instruction)
        {
            consume(instruction.source);
            output(instruction.target);
        }
        void apply_internal(vmir2::load_from_ref const& lfr)
        {
            consume(lfr.from_reference);
            output(lfr.to_value);
        }
        void apply_internal(vmir2::compare_exchange const& op)
        {
            consume(op.target_reference);
            consume(op.expected_reference);
            consume(op.desired_value);
            output(op.result);
        }
        void apply_internal(vmir2::ret const& ret)
        {
        }
        void apply_internal(vmir2::int_add const& add)
        {
            consume(add.a);
            consume(add.b);
            output(add.result);
        }
        void apply_internal(vmir2::int_sub const& sub)
        {
            consume(sub.a);
            consume(sub.b);
            output(sub.result);
        }
        void apply_internal(vmir2::int_mul const& mul)
        {
            consume(mul.a);
            consume(mul.b);
            output(mul.result);
        }
        void apply_internal(vmir2::int_div const& div)
        {
            consume(div.a);
            consume(div.b);
            output(div.result);
        }
        void apply_internal(vmir2::int_mod const& mod)
        {
            consume(mod.a);
            consume(mod.b);
            output(mod.result);
        }
        void apply_internal(vmir2::mut_int_add const& op)
        {
            consume(op.target);
            consume(op.value);
            if (op.old_value.has_value())
            {
                output(*op.old_value);
            }
        }
        void apply_internal(vmir2::mut_int_sub const& op)
        {
            consume(op.target);
            consume(op.value);
            if (op.old_value.has_value())
            {
                output(*op.old_value);
            }
        }
        void apply_internal(vmir2::mut_int_mul const& op)
        {
            consume(op.target);
            consume(op.value);
            if (op.old_value.has_value())
            {
                output(*op.old_value);
            }
        }
        void apply_internal(vmir2::mut_int_div const& op)
        {
            consume(op.target);
            consume(op.value);
            if (op.old_value.has_value())
            {
                output(*op.old_value);
            }
        }
        void apply_internal(vmir2::mut_int_mod const& op)
        {
            consume(op.target);
            consume(op.value);
            if (op.old_value.has_value())
            {
                output(*op.old_value);
            }
        }
        void apply_internal(vmir2::float_add const& add)
        {
            consume(add.a);
            consume(add.b);
            output(add.result);
        }
        void apply_internal(vmir2::float_sub const& sub)
        {
            consume(sub.a);
            consume(sub.b);
            output(sub.result);
        }
        void apply_internal(vmir2::float_mul const& mul)
        {
            consume(mul.a);
            consume(mul.b);
            output(mul.result);
        }
        void apply_internal(vmir2::float_div const& div)
        {
            consume(div.a);
            consume(div.b);
            output(div.result);
        }
        void apply_internal(vmir2::mut_float_add const& op)
        {
            consume(op.target);
            consume(op.value);
        }
        void apply_internal(vmir2::mut_float_sub const& op)
        {
            consume(op.target);
            consume(op.value);
        }
        void apply_internal(vmir2::mut_float_mul const& op)
        {
            consume(op.target);
            consume(op.value);
        }
        void apply_internal(vmir2::mut_float_div const& op)
        {
            consume(op.target);
            consume(op.value);
        }
        void apply_internal(vmir2::float_from_int const& op)
        {
            consume(op.source);
            output(op.result);
        }
        // Bitwise operations state transitions
        void apply_internal(vmir2::bitwise_and const& op)
        {
            consume(op.a);
            consume(op.b);
            output(op.result);
        }
        void apply_internal(vmir2::bitwise_or const& op)
        {
            consume(op.a);
            consume(op.b);
            output(op.result);
        }
        void apply_internal(vmir2::bitwise_xor const& op)
        {
            consume(op.a);
            consume(op.b);
            output(op.result);
        }
        void apply_internal(vmir2::bitwise_nand const& op)
        {
            consume(op.a);
            consume(op.b);
            output(op.result);
        }
        void apply_internal(vmir2::bitwise_nor const& op)
        {
            consume(op.a);
            consume(op.b);
            output(op.result);
        }
        void apply_internal(vmir2::bitwise_nxor const& op)
        {
            consume(op.a);
            consume(op.b);
            output(op.result);
        }
        void apply_internal(vmir2::bitwise_implies const& op)
        {
            consume(op.a);
            consume(op.b);
            output(op.result);
        }
        void apply_internal(vmir2::bitwise_implied const& op)
        {
            consume(op.a);
            consume(op.b);
            output(op.result);
        }
        void apply_internal(vmir2::bitwise_shift_up const& op)
        {
            consume(op.value);
            consume(op.amount);
            output(op.result);
        }
        void apply_internal(vmir2::bitwise_shift_down const& op)
        {
            consume(op.value);
            consume(op.amount);
            output(op.result);
        }
        void apply_internal(vmir2::bitwise_rotate_up const& op)
        {
            consume(op.value);
            consume(op.amount);
            output(op.result);
        }
        void apply_internal(vmir2::bitwise_rotate_down const& op)
        {
            consume(op.value);
            consume(op.amount);
            output(op.result);
        }
        void apply_internal(vmir2::bitwise_inverse const& op)
        {
            consume(op.value);
            output(op.result);
        }
        void apply_internal(vmir2::mut_bitwise_and const& op)
        {
            consume(op.target);
            consume(op.value);
            if (op.old_value.has_value())
            {
                output(*op.old_value);
            }
        }
        void apply_internal(vmir2::mut_bitwise_or const& op)
        {
            consume(op.target);
            consume(op.value);
            if (op.old_value.has_value())
            {
                output(*op.old_value);
            }
        }
        void apply_internal(vmir2::mut_bitwise_xor const& op)
        {
            consume(op.target);
            consume(op.value);
            if (op.old_value.has_value())
            {
                output(*op.old_value);
            }
        }
        void apply_internal(vmir2::mut_bitwise_nand const& op)
        {
            consume(op.target);
            consume(op.value);
            if (op.old_value.has_value())
            {
                output(*op.old_value);
            }
        }
        void apply_internal(vmir2::mut_bitwise_nor const& op)
        {
            consume(op.target);
            consume(op.value);
            if (op.old_value.has_value())
            {
                output(*op.old_value);
            }
        }
        void apply_internal(vmir2::mut_bitwise_nxor const& op)
        {
            consume(op.target);
            consume(op.value);
            if (op.old_value.has_value())
            {
                output(*op.old_value);
            }
        }
        void apply_internal(vmir2::mut_bitwise_implies const& op)
        {
            consume(op.target);
            consume(op.value);
            if (op.old_value.has_value())
            {
                output(*op.old_value);
            }
        }
        void apply_internal(vmir2::mut_bitwise_implied const& op)
        {
            consume(op.target);
            consume(op.value);
            if (op.old_value.has_value())
            {
                output(*op.old_value);
            }
        }
        void apply_internal(vmir2::mut_bitwise_shift_up const& op)
        {
            consume(op.target);
            consume(op.amount);
            if (op.old_value.has_value())
            {
                output(*op.old_value);
            }
        }
        void apply_internal(vmir2::mut_bitwise_shift_down const& op)
        {
            consume(op.target);
            consume(op.amount);
            if (op.old_value.has_value())
            {
                output(*op.old_value);
            }
        }
        void apply_internal(vmir2::mut_bitwise_rotate_up const& op)
        {
            consume(op.target);
            consume(op.amount);
            if (op.old_value.has_value())
            {
                output(*op.old_value);
            }
        }
        void apply_internal(vmir2::mut_bitwise_rotate_down const& op)
        {
            consume(op.target);
            consume(op.amount);
            if (op.old_value.has_value())
            {
                output(*op.old_value);
            }
        }
        void apply_internal(vmir2::store_to_ref const& str)
        {
            consume(str.from_value);
            consume(str.to_reference);
        }
        void apply_internal(vmir2::load_const_int const& lci)
        {
            output(lci.target);
        }
        void apply_internal(vmir2::load_const_enum const& instruction)
        {
            output(instruction.target);
        }
        void apply_internal(vmir2::load_const_float const& lcf)
        {
            output(lcf.target);
        }
        void apply_internal(vmir2::int_cmp const& cmp)
        {
            consume(cmp.a);
            consume(cmp.b);
            output(cmp.result);
        }
        void apply_internal(vmir2::float_cmp const& cmp)
        {
            consume(cmp.a);
            consume(cmp.b);
            output(cmp.result);
        }
        void apply_internal(vmir2::address_cmp const& cmp)
        {
            consume(cmp.a);
            consume(cmp.b);
            output(cmp.result);
        }
        /** Consumes address remainder operands and initializes their SZ result. */
        void apply_internal(vmir2::address_mod const& instruction)
        {
            consume(instruction.address);
            consume(instruction.divisor);
            output(instruction.result);
        }
        void apply_internal(vmir2::type_index_cmp const& cmp)
        {
            consume(cmp.a);
            consume(cmp.b);
            output(cmp.result);
        }
        void apply_internal(vmir2::pointer_cmp const& cmp)
        {
            consume(cmp.a);
            consume(cmp.b);
            output(cmp.result);
        }
        void apply_internal(vmir2::pointer_eq const& cmp)
        {
            consume(cmp.a);
            consume(cmp.b);
            output(cmp.result);
        }
        void apply_internal(vmir2::pointer_ne const& cmp)
        {
            consume(cmp.a);
            consume(cmp.b);
            output(cmp.result);
        }
        void apply_internal(vmir2::cmp_bool const& cmp)
        {
            consume(cmp.ordering);
            output(cmp.result);
        }
        void apply_internal(vmir2::float_ieee_eq const& cmp)
        {
            consume(cmp.a);
            consume(cmp.b);
            output(cmp.result);
        }
        void apply_internal(vmir2::float_ieee_ne const& cmp)
        {
            consume(cmp.a);
            consume(cmp.b);
            output(cmp.result);
        }
        void apply_internal(vmir2::float_ieee_lt const& cmp)
        {
            consume(cmp.a);
            consume(cmp.b);
            output(cmp.result);
        }
        void apply_internal(vmir2::float_ieee_gt const& cmp)
        {
            consume(cmp.a);
            consume(cmp.b);
            output(cmp.result);
        }
        void apply_internal(vmir2::defer_nontrivial_dtor const& dntd)
        {
            readonly(dntd.on_value);
            if (dntd.subobject_context.has_value())
            {
                readonly(dntd.subobject_context->enclosing_object);
            }
            state[dntd.on_value].nontrivial_dtor = dtor_spec{
                .func = dntd.func,
                .args = dntd.args,
                .subobject_context = dntd.subobject_context,
            };
        }
        void apply_internal(vmir2::struct_init_start const& dlg)
        {
            state[dlg.on_value].struct_delegates = dlg.delegates;
            for (vmir2::struct_init_delegate const& delegate : dlg.delegates)
            {
                state[delegate.value].delegate_of = dlg.on_value;
                state[delegate.value].struct_delegate_selector = delegate.selector;
                state[delegate.value].is_projection = false;
                state[delegate.value].persistent_storage = state[dlg.on_value].persistent_storage;
                state[delegate.value].retained_storage = state[dlg.on_value].retained_storage;
                state[delegate.value].storage_valid = true;
                state[delegate.value].stage = slot_stage::dead;
            }

            state[dlg.on_value].storage_valid = true;
            state[dlg.on_value].stage = slot_stage::partial;
        }

        void apply_internal(vmir2::struct_init_finish const& scn)
        {
            std::vector< local_index > bindings = retained_storage_bindings(state, scn.on_value);
            for (std::size_t index = 1; index < bindings.size(); ++index) state.erase(bindings[index]);
            state[scn.on_value].struct_delegates.reset();

            state[scn.on_value].stage = slot_stage::full;
            state[scn.on_value].storage_valid = true;
        }
        void apply_internal(vmir2::copy_reference const& cpr)
        {
            readonly(cpr.from_index);
            output(cpr.to_index);
        }
        /** Initializes a scoped phase guard before invoking lifecycle code. */
        void apply_internal(vmir2::struct_phase_enter const& instruction)
        {
            if (instruction.kind == struct_phase_kind::steady || !slot_info.at(instruction.saved_phase).type.type_is< address_type >())
            {
                throw invalid_instruction_transition_error("lifecycle entry requires an ADDRESS guard and construction or destruction phase");
            }
            if (!state[instruction.on_value].storage_valid)
            {
                throw invalid_instruction_transition_error("lifecycle entry requires allocated object storage");
            }
            if (instruction.subobject_context.has_value())
            {
                if (!state[instruction.subobject_context->enclosing_object].storage_valid)
                    throw invalid_instruction_transition_error("lifecycle entry requires allocated enclosing object storage");
            }
            output(instruction.saved_phase);
            state[instruction.saved_phase].lifecycle_phase = struct_lifecycle_phase{
                .on_value = instruction.on_value, .kind = instruction.kind, .subobject_context = instruction.subobject_context,
                .publish_on_normal_exit = instruction.publish_on_normal_exit};
        }
        /** Consumes a phase guard after its operation and mandatory cleanup complete. */
        void apply_internal(vmir2::struct_phase_leave const& instruction)
        {
            readonly(instruction.saved_phase);
            if (!state[instruction.saved_phase].lifecycle_phase.has_value())
                throw invalid_instruction_transition_error("lifecycle completion requires an active phase guard");
            consume(instruction.saved_phase);
        }
        /** Publishes a completed borrowed construction while preserving enclosing object ownership. */
        void apply_internal(vmir2::complete_initialization const& instruction)
        {
            if (!state[instruction.of].storage_valid || !state[instruction.of].construction_pending)
            {
                throw invalid_instruction_transition_error("initialization completion requires a pending borrowed destination");
            }
            if (instruction.finalize_partial)
            {
                if (!initialization_completed(state, instruction.of))
                {
                    throw invalid_instruction_transition_error("initialization completion requires every owned subobject to be initialized");
                }
                std::vector< local_index > bindings = retained_storage_bindings(state, instruction.of);
                for (std::size_t index = 1; index < bindings.size(); ++index) state.erase(bindings[index]);
                slot_state& value = state[instruction.of];
                value.struct_delegates.reset();
                value.array_delegates.reset();
                value.stage = slot_stage::dead;
            }
            output(instruction.of);
        }
        /** Disarms repeated destructor selection after an owned operation accepts the object. */
        void apply_internal(vmir2::begin_destruction const& instruction)
        {
            readonly(instruction.of);
            if (state[instruction.of].stage != slot_stage::full)
            {
                throw invalid_instruction_transition_error("destruction transfer requires a complete object or reference");
            }
            state[instruction.of].destruction_started = true;
        }
        void apply_internal(vmir2::destroy const& dst)
        {
            consume(dst.of, false, dst.initialization_rollback);
        }
        void apply_internal(vmir2::end_lifetime const& elt)
        {
            if (!state[elt.of].retained_storage)
            {
                consume(elt.of, elt.initialization_rollback, elt.initialization_rollback);
                return;
            }
            std::vector< local_index > bindings = retained_storage_bindings(state, elt.of);
            consume(elt.of, true, elt.initialization_rollback);
            for (local_index index : bindings) state[index] = slot_state{};
        }
        void apply_internal(vmir2::pointer_arith const& par)
        {
            consume(par.from);
            consume(par.offset);
            output(par.result);
        }
        void apply_internal(vmir2::pointer_diff const& pdf)
        {
            consume(pdf.from);
            consume(pdf.to);
            output(pdf.result);
        }
        void apply_internal(vmir2::array_init_start const& ain)
        {
            rpnx::apply_visitor< void >(ain.source,
                                        [&](auto& source)
                                        {
                                            using source_type = std::decay_t< decltype(source) >;
                                            if constexpr (std::is_same_v< source_type, local_index >)
                                            {
                                                vmir2::invocation_args arguments;
                                                arguments.named["INIT"] = ain.initializer;
                                                state[source].array_delegates = arguments;
                                                state[ain.initializer].delegate_of = source;
                                                state[ain.initializer].persistent_storage = state[source].persistent_storage;
                                                state[ain.initializer].retained_storage = state[source].retained_storage;
                                                state[source].storage_valid = true;
                                                state[source].stage = slot_stage::partial;
                                            }
                                            else
                                            {
                                                consume(source.storage_pointer);
                                                consume(source.count);
                                            }
                                        });
            state[ain.initializer].is_projection = false;
            state[ain.initializer].stage = slot_stage::full;
            state[ain.initializer].storage_valid = true;
        }
        void apply_internal(vmir2::array_init_index const& aim)
        {
            // Query initializer; does not consume it; produce result
            readonly(aim.initializer);
            output(aim.result);
        }

        void apply_internal(vmir2::array_init_more const& aim)
        {
            // Query initializer; does not consume it; produce result
            readonly(aim.initializer);
            output(aim.result);
        }
        void apply_internal(vmir2::array_init_element const& aiv)
        {
            // Assign the next array element (not yet constructed) to a target slot.
            // Mark target as an array delegate of the initializer; not alive yet, but storage is valid.
            readonly(aiv.initializer);
            state[aiv.target].array_delegate_of_initializer = aiv.initializer;
            state[aiv.initializer].array_element_delegates.insert(aiv.target);
            state[aiv.target].is_projection = false;
            state[aiv.target].persistent_storage = state[aiv.initializer].persistent_storage;
            state[aiv.target].retained_storage = state[aiv.initializer].retained_storage;
            state[aiv.target].storage_valid = true;
            state[aiv.target].stage = slot_stage::dead;
        }

        /** Transfers the last completed prefix element into an independently cleaned alias. */
        void apply_internal(vmir2::array_deinit_previous const& instruction)
        {
            readonly(instruction.initializer);
            if (state[instruction.target].alive())
            {
                throw invalid_instruction_transition_error("array deinitialization target already alive");
            }
            slot_state& target = state[instruction.target];
            target = slot_state{};
            target.stage = slot_stage::full;
            target.storage_valid = true;
            target.retained_storage = true;
            target.array_delegate_of_initializer = instruction.initializer;
            target.owns_subobject_lifetime = true;
            state[instruction.initializer].array_element_delegates.insert(instruction.target);
        }

        void apply_internal(vmir2::array_init_finish const& aic)
        {
            // Discard any array element delegate slots recorded under the initializer
            auto const it = state.find(aic.initializer);
            if (it != state.end())
            {
                vmir2::invocation_args delegates = it->second.array_delegates.value_or(vmir2::invocation_args{});
                for (auto idx : delegates.positional)
                {
                    state.erase(idx);
                }
                for (auto const& [n, idx] : delegates.named)
                {
                    state.erase(idx);
                }
                for (local_index element : it->second.array_element_delegates) state.erase(element);
                // Finalize the array value that owns this initializer
                auto const& s = it->second;
                if (s.delegate_of.has_value())
                {
                    auto arr_idx = *s.delegate_of;
                    state[arr_idx].stage = slot_stage::full;
                    assert(state[arr_idx].storage_valid == true);
                    // Clear delegates on the array to remove the initializer reference
                    state[arr_idx].array_delegates.reset();
                }
                // Mark initializer slot as no longer needed
                state.erase(aic.initializer);
            }
        }

        void readonly(local_index idx)
        {
            if (state[idx].stage == slot_stage::dead || !state[idx].storage_valid) [[unlikely]]
            {
                throw invalid_instruction_transition_error("readonly input not alive state");
            }
        }
        void publish_fusion_state(local_index target)
        {
            if (is_ref(slot_info.at(static_cast< std::uint64_t >(target)).type))
            {
                readonly(target);
                return;
            }

            slot_state& target_state = state[target];
            if (target_state.stage != slot_stage::dead || !target_state.storage_valid) [[unlikely]]
            {
                throw invalid_instruction_transition_error("fusion state publication requires a live reference or dead initialized output storage");
            }
            target_state.stage = slot_stage::full;
        }
        void consume(local_index idx, bool end_partial_storage = false, bool initialization_rollback = false)
        {
            auto validate_parameter = [&](routine_parameter const& parameter)
            {
                if (!initialization_rollback && parameter.local_index == idx && parameter.type.type_is< nvalue_slot >())
                {
                    throw invalid_instruction_transition_error("An initialization output cannot be consumed before normal return");
                }
            };
            for (routine_parameter const& parameter : routine_params.positional)
            {
                validate_parameter(parameter);
            }
            for (std::pair< std::string const, routine_parameter > const& parameter : routine_params.named)
            {
                validate_parameter(parameter.second);
            }
            if (!initialization_rollback && state[idx].delegate_of.has_value())
            {
                throw invalid_instruction_transition_error("delegates are not consumable");
            }
            if ((!end_partial_storage && state[idx].stage != slot_stage::full) || !state[idx].storage_valid) [[unlikely]]
            {
                throw invalid_instruction_transition_error("consume input slot " + std::to_string(idx) + " has stage " +
                    std::to_string(static_cast< unsigned >(state[idx].stage)) + " and storage " + (state[idx].storage_valid ? "allocated" : "unallocated"));
            }
            state[idx].stage = slot_stage::dead;
            state[idx].storage_valid = false;
            state[idx].nontrivial_dtor.reset();
            state[idx].lifecycle_phase.reset();
            state[idx].destruction_started = false;
            state[idx].construction_pending = false;
            state[idx].destroy_delegate = false;
            state[idx].is_projection = false;
            state[idx].persistent_storage = false;
            state[idx].retained_storage = false;
            state[idx].storage_origin.reset();
        }
        void output(local_index idx)
        {
            if (state[idx].alive()) [[unlikely]]
            {
                throw invalid_instruction_transition_error("output already set");
            }
            if (state[idx].is_projection && !state[idx].retained_storage)
            {
                if (!state[idx].storage_valid) [[unlikely]]
                {
                    throw invalid_instruction_transition_error("projection output not valid");
                }
                state.erase(idx);
                return;
            }
            if (!state[idx].storage_valid)
            {
                state[idx].is_projection = false;
            }
            state[idx].stage = slot_stage::full;
            state[idx].destruction_started = false;
            state[idx].construction_pending = false;
            if (!state[idx].storage_valid && state[idx].delegate_of.has_value()) [[unlikely]]
            {
                throw invalid_instruction_transition_error("output not valid");
            }
            state[idx].storage_valid = true;
        }
    };

} // namespace quxlang::vmir2

#endif // RPNX_QUXLANG_STATE_ENGINE_HEADER
