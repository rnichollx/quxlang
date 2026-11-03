// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/vmir2/coroutine_lowering.hpp>

#include <quxlang/data/coroutine_types.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/vmir2/coroutine_vmir_analysis.hpp>
#include <quxlang/vmir2/state_engine.hpp>
#include <quxlang/vmir2/lifetime_snapshot_graph.hpp>

#include <quxlang/vmir2/coroutine_output_initialization.hpp>

#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <tuple>
#include <utility>

namespace quxlang::vmir2
{
    /** Generates frame bindings and ordinary control flow while retaining logical local identities. */
    class coroutine_resume_lowering
    {
      private:
        functanoid_routine3 const& logical;
        coroutine_frame_info const& frame;
        coroutine_body const& body;
        functanoid_routine3 output;
        local_index return_slot;
        routine_parameters replay_parameters;
        bool cancellation_driver;
        std::map< block_index, block_index > resume_entries;
        std::map< block_index, block_index > suspension_exits;
        std::map< local_index, local_index > saved_address_slots;
        lifetime_snapshot_transform< state_map > activation_snapshots;
        lifetime_snapshot_transform< state_map > saved_address_snapshots;
        coroutine_output_initialization initialization;
        using completion_key = std::tuple< std::optional< local_index >, std::optional< type_symbol >, std::optional< type_symbol >, bool >;
        std::map< completion_key, block_index > completion_blocks;

        /** Allocates one activation-local compiler temporary. */
        auto append_local(type_symbol type) -> local_index
        {
            local_index index(output.local_types.size());
            output.local_types.push_back({.type = std::move(type)});
            return index;
        }

        /** Projects a frame field through a consumable copy of the execution parameter. */
        auto append_field_reference(executable_block& block, std::size_t ordinal) -> local_index
        {
            struct_field const& field = frame.fields.at(ordinal);
            local_index receiver = append_local(output.local_types.at(body.frame).type);
            local_index reference = append_local(make_mref(field.type));
            block.instructions.push_back(copy_reference{.from_index = body.frame, .to_index = receiver});
            block.instructions.push_back(access_field{.base_index = receiver, .store_index = reference, .field_name = field.name});
            return reference;
        }

        /** Publishes an initialized, suspended, or completed continuation ordinal. */
        void append_continuation_write(executable_block& block, std::uint32_t ordinal)
        {
            local_index reference = append_field_reference(block, 0);
            local_index value = append_local(frame.fields.at(0).type);
            block.instructions.push_back(load_const_int{.target = value, .value = std::to_string(ordinal)});
            block.instructions.push_back(store_to_ref{.from_value = value, .to_reference = reference});
        }

        /** Reads a completed typed-storage scalar without transferring its persistent lifetime. */
        auto append_capture_read(executable_block& block, local_index capture) -> local_index
        {
            local_index storage_reference = append_field_reference(block, frame.local_fields.at(capture));
            type_symbol const& type = logical.local_types.at(capture).type;
            local_index reference = append_local(make_mref(type));
            local_index value = append_local(type);
            block.instructions.push_back(storage_pun{.from_storage = storage_reference, .as_type = type, .to_reference = reference});
            block.instructions.push_back(load_from_ref{.from_reference = reference, .to_value = value});
            return value;
        }

        /** Publishes the invocation's first cancellation request without changing an existing request. */
        void append_cancellation_request(executable_block& block)
        {
            local_index capture = *body.cancellation_state;
            local_index storage_reference = append_field_reference(block, frame.local_fields.at(capture));
            type_symbol const& type = logical.local_types.at(capture).type;
            local_index reference = append_local(make_mref(type));
            local_index value = append_local(type);
            block.instructions.push_back(storage_pun{.from_storage = storage_reference, .as_type = type, .to_reference = reference});
            block.instructions.push_back(load_const_int{.target = value, .value = "1"});
            block.instructions.push_back(store_to_ref{.from_value = value, .to_reference = reference});
        }

        /** Dispatches only initialized or suspended invocations through cancellation-request handling. */
        void generate_cancellation_entry(block_index invalid, block_index completed)
        {
            executable_block pending{
                .entry_state = activation_state({}),
                .instructions = {load_const_bool{.target = return_slot, .value = false}},
                .terminator = ret{},
                .dbg_name = "COROUTINE_CANCEL_PENDING",
            };
            block_index pending_index(output.blocks.size());
            output.blocks.push_back(std::move(pending));

            std::vector< block_index > destinations{invalid, append_resume_entry(block_index(0))};
            for (coroutine_continuation const& continuation : body.continuations)
            {
                destinations.push_back(!continuation.shielded && continuation.cancellation.has_value()
                    ? append_resume_entry(*continuation.cancellation) : pending_index);
            }
            executable_block active{.entry_state = activation_state({}), .dbg_name = "COROUTINE_CANCEL_DISPATCH"};
            local_index active_reference = append_field_reference(active, 0);
            local_index active_ordinal = append_local(frame.fields.at(0).type);
            active.instructions.push_back(load_from_ref{.from_reference = active_reference, .to_value = active_ordinal});
            active.terminator = tablebranch{.index = active_ordinal, .targets = std::move(destinations), .default_target = completed};
            block_index active_index(output.blocks.size());
            output.blocks.push_back(std::move(active));

            executable_block request{.entry_state = activation_state({}), .dbg_name = "COROUTINE_CANCEL_REQUEST"};
            append_cancellation_request(request);
            request.terminator = jump{.target = active_index};
            block_index request_index(output.blocks.size());
            output.blocks.push_back(std::move(request));

            executable_block inspect{.entry_state = activation_state({}), .dbg_name = "COROUTINE_CANCEL_ATTEMPT"};
            local_index attempt = append_capture_read(inspect, *body.cancellation_state);
            inspect.terminator = tablebranch{.index = attempt, .targets = {request_index, active_index}, .default_target = pending_index};
            block_index inspect_index(output.blocks.size());
            output.blocks.push_back(std::move(inspect));

            executable_block entry{.entry_state = activation_state({}), .dbg_name = "COROUTINE_CANCEL_ENTRY"};
            local_index reference = append_field_reference(entry, 0);
            local_index ordinal = append_local(frame.fields.at(0).type);
            entry.instructions.push_back(load_from_ref{.from_reference = reference, .to_value = ordinal});
            std::vector< block_index > active_states(body.continuations.size() + 2, inspect_index);
            active_states.front() = invalid;
            entry.terminator = tablebranch{.index = ordinal, .targets = std::move(active_states), .default_target = completed};
            output.blocks[0] = std::move(entry);
        }

        /** Selects the alias stages whose physical address belongs to the frame. */
        bool retains_alias_address(local_index local, slot_state const& slot) const
        {
            if (frame.saved_cleanup_alias_addresses.contains(local)) return slot.storage_valid;
            return frame.saved_array_element_addresses.contains(local) && slot.stage == slot_stage::full && !slot.owns_subobject_lifetime;
        }

        /** Rewrites each shared lifetime version once and attaches activation parameters. */
        auto activation_state(state_map const& state) -> state_map
        {
            state_map::snapshot logical_root = activation_snapshots.apply(state.snapshot_root(),
                [&](state_map::value_type const& entry) -> std::optional< state_map::value_type >
                {
                    slot_state slot = entry.second;
                    bool root = frame.local_fields.contains(entry.first) && !saved_address_slots.contains(entry.first);
                    bool alias = slot.delegate_of.has_value() || slot.array_delegate_of_initializer.has_value() || slot.storage_origin.has_value() ||
                        body.cleanup_aliases.contains(entry.first);
                    if (root || alias) slot.retained_storage = true;
                    if (root && !(slot.persistent_storage && slot.is_projection))
                    {
                        slot.persistent_storage = false;
                        slot.is_projection = false;
                        if (frame.borrowed_parameters.contains(entry.first) && frame.borrowed_parameters.at(entry.first).type_is< dvalue_slot >())
                        {
                            slot.destroy_delegate = entry.second.alive();
                        }
                    }
                    return state_map::value_type(entry.first, std::move(slot));
                });
            state_map::snapshot saved_root = saved_address_snapshots.apply(state.snapshot_root(),
                [&](state_map::value_type const& entry) -> std::optional< state_map::value_type >
                {
                    if (!retains_alias_address(entry.first, entry.second))
                    {
                        return std::nullopt;
                    }
                    return state_map::value_type(saved_address_slots.at(entry.first), slot_state{.stage = slot_stage::full, .storage_valid = true, .retained_storage = true});
                });
            state_map result = state_map::from_snapshot(lifetime_snapshot_transform< state_map >::concatenate(std::move(logical_root), std::move(saved_root)));
            result[body.frame] = slot_state{.stage = slot_stage::full, .storage_valid = true};
            result[body.controller] = slot_state{.stage = slot_stage::full, .storage_valid = true};
            result[return_slot] = slot_state{.storage_valid = true};
            return result;
        }

        /** Obtains the first logical block's parameter state when it uses the implicit entry convention. */
        auto logical_entry(std::size_t index) const -> state_map
        {
            state_map state = logical.blocks.at(index).entry_state;
            if (index == 0 && state.empty())
            {
                codegen_state_engine engine(state, logical.local_types, logical.parameters);
                engine.apply_entry();
            }
            return state;
        }

        /** Selects a shared continuation snapshot whose payload bindings resolve on demand. */
        auto append_resume_entry(block_index destination) -> block_index
        {
            std::map< block_index, block_index >::const_iterator existing = resume_entries.find(destination);
            if (existing != resume_entries.end()) return existing->second;
            executable_block block{
                .entry_state = activation_state(logical_entry(static_cast< std::uint64_t >(destination))),
                .terminator = jump{.target = block_index(static_cast< std::uint64_t >(destination) + 1)},
                .dbg_name = "COROUTINE_RESUME_ENTRY",
            };
            block_index index(output.blocks.size());
            output.blocks.push_back(std::move(block));
            resume_entries.emplace(destination, index);
            return index;
        }

        /** Moves control-flow destinations past the generated dispatch block. */
        static void relocate_targets(vm_terminator& terminator)
        {
            auto shift = [](block_index& target) { target = block_index(static_cast< std::uint64_t >(target) + 1); };
            rpnx::apply_visitor< void >(terminator, [&](auto& instruction)
            {
                using instruction_type = std::decay_t< decltype(instruction) >;
                if constexpr (std::is_same_v< instruction_type, jump >) shift(instruction.target);
                else if constexpr (std::is_same_v< instruction_type, branch >)
                {
                    shift(instruction.target_true);
                    shift(instruction.target_false);
                }
                else if constexpr (std::is_same_v< instruction_type, tablebranch >)
                {
                    for (block_index& target : instruction.targets) shift(target);
                    shift(instruction.default_target);
                }
                else if constexpr (std::is_same_v< instruction_type, policy_branch >)
                {
                    for (block_index& target : instruction.targets) shift(target);
                }
                else if constexpr (std::is_same_v< instruction_type, runtime_constexpr >)
                {
                    shift(instruction.target_constexpr);
                    shift(instruction.target_native);
                }
                else if constexpr (std::is_same_v< instruction_type, initguard_try_acquire >)
                {
                    shift(instruction.target_acquired);
                    shift(instruction.target_already_initialized);
                }
            });
        }

        /** Expands construction bindings and continuation saves without cloning live state per instruction. */
        auto lower_body_block(std::size_t index) -> executable_block
        {
            executable_block const& source = logical.blocks.at(index);
            state_map state = logical_entry(index);
            executable_block block{
                .entry_state = activation_state(state),
                .dbg_name = source.dbg_name,
                .catcher = source.catcher,
            };
            if (block.catcher.has_value()) block.catcher->handler = block_index(static_cast< std::uint64_t >(block.catcher->handler) + 1);
            codegen_state_engine engine(state, logical.local_types, replay_parameters);
            for (vm_instruction const& instruction : source.instructions)
            {
                if (instruction.type_is< co_save >())
                {
                    std::uint64_t continuation = instruction.as< co_save >().continuation;
                    if (continuation >= body.continuations.size()) throw compiler_bug("Invalid coroutine continuation checkpoint");
                    append_continuation_write(block, static_cast< std::uint32_t >(continuation + 2));
                    continue;
                }
                std::vector< local_index > touched = coroutine_instruction_locals(instruction, state);
                std::vector< bool > allocated;
                std::vector< bool > addressed;
                allocated.reserve(touched.size());
                addressed.reserve(touched.size());
                for (local_index local : touched)
                {
                    state_map::const_iterator before = state.find(local);
                    allocated.push_back(before != state.end() && (before->second.storage_valid || before->second.alive()));
                    addressed.push_back(before != state.end() && retains_alias_address(local, before->second));
                }
                bool ownership_transfer = instruction.type_is< lifetime_transfer >() || instruction.type_is< lifetime_import >();
                engine.apply_validated(instruction);
                for (std::size_t position = 0; position < touched.size(); ++position)
                {
                    local_index local = touched[position];
                    if (ownership_transfer || allocated[position] || !frame.local_fields.contains(local) || saved_address_slots.contains(local)) continue;
                    state_map::const_iterator after = state.find(local);
                    if (after == state.end() || (!after->second.storage_valid && !after->second.alive())) continue;
                    local_index reference = append_field_reference(block, frame.local_fields.at(local));
                    block.instructions.push_back(storage_init_start{.on_storage = reference, .target_value = local, .persistent = true, .owns_lifetime = true});
                    block.instructions.push_back(end_lifetime{.of = reference});
                }
                if (instruction.type_is< lifetime_transfer >())
                {
                    lifetime_transfer transfer = instruction.as< lifetime_transfer >();
                    transfer.state = activation_state(transfer.state);
                    block.instructions.push_back(std::move(transfer));
                }
                else if (instruction.type_is< lifetime_import >())
                {
                    lifetime_import imported = instruction.as< lifetime_import >();
                    state_map active = activation_state(state);
                    if (saved_address_slots.contains(imported.slot))
                    {
                        local_index address = saved_address_slots.at(imported.slot);
                        if (imported.state == slot_state{} && frame.saved_cleanup_alias_addresses.contains(imported.slot))
                        {
                            block.instructions.push_back(lifetime_import{.slot = address,
                                .state = slot_state{.stage = slot_stage::full, .storage_valid = true, .retained_storage = true}});
                            block.instructions.push_back(end_lifetime{.of = address});
                        }
                        else if (active.contains(address)) block.instructions.push_back(lifetime_import{.slot = address, .state = active.at(address)});
                    }
                    imported.state = active.contains(imported.slot) ? active.at(imported.slot) : slot_state{};
                    block.instructions.push_back(std::move(imported));
                }
                else block.instructions.push_back(instruction);
                if (ownership_transfer) continue;
                for (std::size_t position = 0; position < touched.size(); ++position)
                {
                    local_index local = touched[position];
                    if (!saved_address_slots.contains(local)) continue;
                    state_map::const_iterator after = state.find(local);
                    bool address_required = after != state.end() && retains_alias_address(local, after->second);
                    if (addressed[position] && !address_required)
                    {
                        block.instructions.push_back(end_lifetime{.of = saved_address_slots.at(local)});
                    }
                    else if (!addressed[position] && address_required)
                    {
                        local_index reference = append_field_reference(block, frame.local_fields.at(local));
                        local_index saved_address = saved_address_slots.at(local);
                        block.instructions.push_back(storage_init_start{.on_storage = reference, .target_value = saved_address, .persistent = true, .owns_lifetime = true});
                        block.instructions.push_back(end_lifetime{.of = reference});
                        block.instructions.push_back(make_reference{.value_index = local, .reference_index = saved_address,
                            .storage_only = after->second.stage == slot_stage::dead});
                    }
                }
            }
            if (!source.terminator.has_value()) throw compiler_bug("Coroutine VMIR contains an unterminated block");
            if (source.terminator->type_is< co_suspend >())
            {
                std::uint64_t continuation = source.terminator->as< co_suspend >().continuation;
                if (continuation >= body.continuations.size()) throw compiler_bug("Invalid coroutine suspension");
                block_index destination = body.continuations[continuation].resume;
                std::map< block_index, block_index >::const_iterator existing = suspension_exits.find(destination);
                block_index target;
                if (existing != suspension_exits.end()) target = existing->second;
                else
                {
                    executable_block suspended{
                        .entry_state = activation_state(logical_entry(static_cast< std::uint64_t >(destination))),
                        .instructions = {load_const_bool{.target = return_slot, .value = false}},
                        .terminator = ret{.retain_storage = true},
                        .dbg_name = "COROUTINE_SUSPEND",
                    };
                    target = block_index(output.blocks.size());
                    output.blocks.push_back(std::move(suspended));
                    suspension_exits.emplace(destination, target);
                }
                block.terminator = jump{.target = target};
            }
            else if (source.terminator->type_is< initguard_try_acquire >() &&
                frame.local_fields.contains(source.terminator->as< initguard_try_acquire >().target_lock))
            {
                initguard_try_acquire acquire = source.terminator->as< initguard_try_acquire >();
                local_index lock = acquire.target_lock;
                local_index acquired_lock = append_local(logical.local_types.at(lock).type);
                state_map acquired_state = activation_state(state);
                acquired_state[acquired_lock] = slot_state{.stage = slot_stage::full, .storage_valid = true};
                executable_block captured{.entry_state = std::move(acquired_state), .dbg_name = "COROUTINE_CAPTURE_INITGUARD"};
                local_index reference = append_field_reference(captured, frame.local_fields.at(lock));
                captured.instructions.push_back(storage_init_start{.on_storage = reference, .target_value = lock, .persistent = true, .owns_lifetime = true});
                captured.instructions.push_back(end_lifetime{.of = reference});
                captured.instructions.push_back(relocate_value{.source = acquired_lock, .target = lock});
                captured.terminator = jump{.target = block_index(static_cast< std::uint64_t >(acquire.target_acquired) + 1)};
                acquire.target_lock = acquired_lock;
                acquire.target_acquired = block_index(output.blocks.size());
                acquire.target_already_initialized = block_index(static_cast< std::uint64_t >(acquire.target_already_initialized) + 1);
                output.blocks.push_back(std::move(captured));
                block.terminator = std::move(acquire);
            }
            else if (source.terminator->type_is< co_complete >())
            {
                co_complete const& complete = source.terminator->as< co_complete >();
                bool successful = !complete.exception.has_value() && !complete.exceptional_exit && !complete.cancelled_function.has_value();
                if (successful && !initialization.completed(state))
                {
                    throw invalid_instruction_transition_error("Coroutine completion requires initialized NVALUE destinations with complete ownership evidence");
                }
                completion_key key{complete.exception, complete.exception_function, complete.cancelled_function, complete.exceptional_exit};
                std::map< completion_key, block_index >::const_iterator existing = completion_blocks.find(key);
                if (existing != completion_blocks.end())
                {
                    block.terminator = jump{.target = existing->second};
                    return block;
                }
                state_map terminal_state = activation_state({});
                if (successful)
                {
                    for (std::pair< local_index const, type_symbol > const& parameter : frame.borrowed_parameters)
                    {
                        if (!parameter.second.type_is< nvalue_slot >()) continue;
                        terminal_state[parameter.first] = slot_state{
                            .stage = slot_stage::full,
                            .storage_valid = true,
                            .construction_pending = true,
                            .persistent_storage = true,
                            .retained_storage = true,
                        };
                    }
                }
                if (complete.exception.has_value())
                {
                    terminal_state[*complete.exception] = state.at(*complete.exception);
                    if (frame.local_fields.contains(*complete.exception))
                    {
                        terminal_state[*complete.exception].retained_storage = true;
                        terminal_state[*complete.exception].is_projection = false;
                    }
                }
                executable_block terminal{.entry_state = std::move(terminal_state), .dbg_name = "COROUTINE_COMPLETE"};
                if (successful)
                {
                    for (std::pair< local_index const, type_symbol > const& parameter : frame.borrowed_parameters)
                    {
                        if (parameter.second.type_is< nvalue_slot >())
                        {
                            terminal.instructions.push_back(complete_initialization{.of = parameter.first, .finalize_partial = true});
                        }
                    }
                }
                for (std::pair< local_index const, type_symbol > const& parameter : frame.borrowed_parameters)
                {
                    local_index storage_reference = append_field_reference(terminal, frame.local_fields.at(parameter.first));
                    type_symbol target = parameter.second.type_is< nvalue_slot >() ? parameter.second.as< nvalue_slot >().target : parameter.second.as< dvalue_slot >().target;
                    local_index saved_reference = append_local(make_mref(target));
                    terminal.instructions.push_back(storage_rebind{.on_storage = storage_reference, .target_value = saved_reference, .owns_lifetime = true});
                    terminal.instructions.push_back(end_lifetime{.of = storage_reference});
                    terminal.instructions.push_back(end_lifetime{.of = saved_reference});
                }
                if (complete.cancelled_function.has_value())
                {
                    local_index controller = append_local(output.local_types.at(body.controller).type);
                    local_index argument = append_local(output.local_types.at(body.frame).type);
                    terminal.instructions.push_back(copy_reference{.from_index = body.controller, .to_index = controller});
                    terminal.instructions.push_back(copy_reference{.from_index = body.frame, .to_index = argument});
                    terminal.instructions.push_back(invoke{.what = *complete.cancelled_function, .args = {.named = {{"THIS", controller}, {"ARG", argument}}}});
                }
                append_continuation_write(terminal, std::numeric_limits< std::uint32_t >::max());
                if (complete.exception.has_value())
                {
                    if (!complete.exception_function.has_value()) throw compiler_bug("Coroutine exception completion requires an owned-exception propagation function");
                    local_index argument = append_local(make_cref(logical.local_types.at(*complete.exception).type));
                    terminal.instructions.push_back(make_reference{.value_index = *complete.exception, .reference_index = argument});
                    terminal.instructions.push_back(invoke{.what = *complete.exception_function, .args = {.named = {{"ARG", argument}}}});
                    terminal.terminator = unreachable{};
                }
                else
                {
                    terminal.instructions.push_back(load_const_bool{.target = return_slot, .value = true});
                    terminal.terminator = ret{};
                }
                block_index target(output.blocks.size());
                output.blocks.push_back(std::move(terminal));
                completion_blocks.emplace(std::move(key), target);
                block.terminator = jump{.target = target};
            }
            else
            {
                block.terminator = source.terminator;
                relocate_targets(*block.terminator);
            }
            return block;
        }

      public:
        /** Initializes the driver signature while preserving logical storage indices. */
        coroutine_resume_lowering(functanoid_routine3 const& routine, coroutine_frame_info const& layout, bool cancel)
            : logical(routine), frame(layout), body(*routine.coroutine), output(routine), cancellation_driver(cancel), initialization(routine, layout.borrowed_parameters)
        {
            output.coroutine.reset();
            for (local_index local : frame.saved_array_element_addresses)
            {
                saved_address_slots.emplace(local, append_local(make_mref(logical.local_types.at(local).type)));
            }
            for (local_index local : frame.saved_cleanup_alias_addresses)
            {
                if (!saved_address_slots.contains(local)) saved_address_slots.emplace(local, append_local(ptrref_type{
                    .target = logical.local_types.at(local).type, .ptr_class = pointer_class::ref, .qual = qualifier::mut}));
            }
            for (std::pair< local_index const, std::size_t > const& field : frame.local_fields)
            {
                retained_storage_kind kind = frame.borrowed_parameters.contains(field.first) ? retained_storage_kind::reference : retained_storage_kind::object;
                if (frame.saved_array_element_addresses.contains(field.first)) kind = retained_storage_kind::completed_array_reference;
                if (frame.saved_cleanup_alias_addresses.contains(field.first)) kind = retained_storage_kind::captured_alias_reference;
                output.retained_bindings.emplace(field.first, retained_storage_binding{.frame = body.frame, .field_name = frame.fields.at(field.second).name, .kind = kind});
                if (saved_address_slots.contains(field.first))
                {
                    output.retained_bindings.at(field.first).saved_reference = saved_address_slots.at(field.first);
                    output.retained_bindings.emplace(saved_address_slots.at(field.first), retained_storage_binding{.frame = body.frame, .field_name = frame.fields.at(field.second).name});
                }
            }
            for (std::pair< local_index const, local_index > const& alias : body.cleanup_aliases)
            {
                std::size_t field = frame.local_fields.at(alias.second);
                output.retained_bindings.emplace(alias.first, retained_storage_binding{
                    .frame = body.frame,
                    .field_name = frame.fields.at(field).name,
                    .kind = retained_storage_kind::captured_alias_reference,
                    .saved_reference = alias.second,
                });
            }
            output.parameters = {};
            output.parameters.named.emplace("THIS", routine_parameter{.type = output.local_types.at(body.frame).type, .local_index = body.frame});
            output.parameters.named.emplace("CONTROLLER", routine_parameter{.type = output.local_types.at(body.controller).type, .local_index = body.controller});
            return_slot = append_local(bool_type{});
            output.parameters.named.emplace("RETURN", routine_parameter{.type = nvalue_slot{.target = bool_type{}}, .local_index = return_slot});
            output.blocks.clear();
            output.blocks.resize(logical.blocks.size() + 1);
            output.block_names.clear();
            for (std::pair< block_index const, std::string > const& name : logical.block_names)
            {
                output.block_names.emplace(block_index(static_cast< std::uint64_t >(name.first) + 1), name.second);
            }
        }

        /** Emits ordinary VMIR procedures with no remaining coroutine instructions or terminators. */
        auto run() -> functanoid_routine3
        {
            for (std::size_t index = 0; index < logical.blocks.size(); ++index) output.blocks[index + 1] = lower_body_block(index);
            executable_block invalid{.entry_state = activation_state({}), .terminator = unreachable{}, .dbg_name = "COROUTINE_INVALID_STATE"};
            block_index invalid_index(output.blocks.size());
            output.blocks.push_back(std::move(invalid));
            executable_block completed{
                .entry_state = activation_state({}),
                .instructions = {load_const_bool{.target = return_slot, .value = true}},
                .terminator = ret{},
                .dbg_name = "COROUTINE_ALREADY_COMPLETE",
            };
            block_index completed_index(output.blocks.size());
            output.blocks.push_back(std::move(completed));
            if (cancellation_driver)
            {
                generate_cancellation_entry(invalid_index, completed_index);
                return std::move(output);
            }
            std::vector< block_index > destinations{invalid_index, append_resume_entry(block_index(0))};
            for (coroutine_continuation const& continuation : body.continuations) destinations.push_back(append_resume_entry(continuation.resume));
            executable_block entry{.entry_state = activation_state({}), .dbg_name = "COROUTINE_DISPATCH"};
            local_index reference = append_field_reference(entry, 0);
            local_index ordinal = append_local(frame.fields.at(0).type);
            entry.instructions.push_back(load_from_ref{.from_reference = reference, .to_value = ordinal});
            entry.terminator = tablebranch{.index = ordinal, .targets = std::move(destinations), .default_target = completed_index};
            output.blocks[0] = std::move(entry);
            return std::move(output);
        }
    };
}

auto quxlang::vmir2::lower_coroutine_resume(functanoid_routine3 const& logical, coroutine_frame_info const& frame) -> functanoid_routine3
{
    if (!logical.coroutine.has_value() || logical.blocks.empty()) throw compiler_bug("RESUME lowering requires a logical coroutine body");
    if (logical.coroutine->continuations.size() > std::numeric_limits< std::uint32_t >::max() - 2)
    {
        throw semantic_compilation_error("Coroutine continuation count exceeds its frame encoding");
    }
    coroutine_resume_lowering lowering(logical, frame, false);
    return lowering.run();
}

auto quxlang::vmir2::lower_coroutine_try_cancel(functanoid_routine3 const& logical, coroutine_frame_info const& frame) -> functanoid_routine3
{
    if (!logical.coroutine.has_value() || logical.blocks.empty()) throw compiler_bug("TRY_CANCEL lowering requires a logical coroutine body");
    if (logical.coroutine->cancellation_mode == coroutine_cancellation_mode::none || !logical.coroutine->cancellation_state.has_value())
    {
        throw semantic_compilation_error("TRY_CANCEL requires a CANCELLABLE coroutine specialization");
    }
    if (logical.coroutine->continuations.size() > std::numeric_limits< std::uint32_t >::max() - 2)
    {
        throw semantic_compilation_error("Coroutine continuation count exceeds its frame encoding");
    }
    coroutine_resume_lowering lowering(logical, frame, true);
    return lowering.run();
}
