// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_LIFETIME_CLEANUP_PROGRAM_HEADER_GUARD
#define QUXLANG_VMIR2_LIFETIME_CLEANUP_PROGRAM_HEADER_GUARD

#include <quxlang/vmir2/lifetime_snapshot_graph.hpp>
#include <quxlang/vmir2/lifetime_transition_graph.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <set>
#include <vector>

namespace quxlang::vmir2
{
    /**
     * Retains cleanup transitions as shared lifetime roots and a resumable cursor.
     * Construction visits each represented snapshot node once. A transition adds
     * two root indices without expanding its source-to-destination difference.
     * Cursor evaluation preserves the generic lifetime planner's action ordering.
     */
    class lifetime_cleanup_program
    {
      public:
        using snapshot_graph = lifetime_snapshot_graph< state_map >;
        using node_index = std::size_t;

        /** Identifies the immutable states and exit contract of one cleanup edge. */
        struct transition
        {
            std::optional< node_index > source;
            std::optional< node_index > target;
            lifetime_transition_kind kind = lifetime_transition_kind::control_flow;
        };

        /** Stores the progress required to resume a partially executed transition. */
        struct cursor
        {
            std::size_t transition_index = 0;
            std::optional< std::uint64_t > before;
            std::optional< node_index > pending_end;
            bool ordinary = false;
            bool publication = false;
            bool completed = false;
        };

        /** Associates a selected action with its statically typed source slot version. */
        struct selected_action
        {
            lifetime_action action;
            node_index version = 0;
        };

        /** Binds the routine's stable local types and destructor selection. */
        explicit lifetime_cleanup_program(functanoid_routine3 const& routine) : routine_(routine)
        {
            auto remember = [&](routine_parameter const& parameter)
            {
                if (parameter.type.type_is< dvalue_slot >()) destroy_parameters_.insert(parameter.local_index);
            };
            for (routine_parameter const& parameter : routine.parameters.positional) remember(parameter);
            for (std::pair< std::string const, routine_parameter > const& parameter : routine.parameters.named) remember(parameter.second);
        }

        /** Registers an edge using shared source and target snapshots. */
        std::size_t append(state_map const& source, state_map const& target, lifetime_transition_kind kind)
        {
            transition entry{.source = graph_.append(source.snapshot_root()), .target = graph_.append(target.snapshot_root()), .kind = kind};
            source_nodes_.resize(graph_.nodes().size());
            mark_source(entry.source);
            transitions_.push_back(std::move(entry));
            return transitions_.size() - 1;
        }

        /** Exposes deterministic postorder nodes for static dispatch generation. */
        snapshot_graph const& graph() const { return graph_; }

        /** Identifies nodes reachable from a cleanup source independently of target-only metadata. */
        std::vector< bool > const& source_nodes() const { return source_nodes_; }

        /** Lists each newly reachable source leaf once, including former target-only versions. */
        std::vector< node_index > const& source_leaves() const { return source_leaves_; }

        /** Combines every exit contract that can select an action for one typed leaf. */
        std::uint64_t action_mask(node_index index) const
        {
            state_map::value_type const& entry = leaf(index);
            slot_state const state = storage_state(entry.first, entry.second);
            std::uint64_t result = 0;
            for (lifetime_transition_kind kind : {lifetime_transition_kind::control_flow, lifetime_transition_kind::normal_exit,
                lifetime_transition_kind::exceptional_exit, lifetime_transition_kind::instruction, lifetime_transition_kind::retained_exit})
                result |= possible_actions(entry.first, state, kind);
            return result;
        }

        /** Exposes the constant-size descriptors belonging to cleanup edges. */
        std::vector< transition > const& transitions() const { return transitions_; }

        /** Defines the target-independent integer columns of one immutable cursor node. */
        enum class column : std::size_t
        {
            left, right, minimum_key, branch_bit, lifetime_flags, owner, owns_lifetime,
            control_flow_actions, normal_exit_actions, exceptional_exit_actions,
            instruction_actions, retained_exit_actions, count
        };

        using encoded_node = std::array< std::uint64_t, static_cast< std::size_t >(column::count) >;
        static constexpr std::uint64_t delegate_action = std::uint64_t{1} << 32;
        static constexpr std::uint64_t release_alias = std::uint64_t{1} << 33;
        static constexpr std::uint64_t publication_action = std::uint64_t{1} << 34;

        /** Emits each shared node once; zero identifies the empty tree. */
        std::vector< encoded_node > encode() const
        {
            std::vector< encoded_node > result(1);
            result.reserve(graph_.nodes().size() + 1);
            for (snapshot_graph::node const& node : graph_.nodes())
            {
                encoded_node record{};
                record[static_cast< std::size_t >(column::left)] = node.left.has_value() ? *node.left + 1 : 0;
                record[static_cast< std::size_t >(column::right)] = node.right.has_value() ? *node.right + 1 : 0;
                record[static_cast< std::size_t >(column::minimum_key)] = node.source->minimum_key;
                record[static_cast< std::size_t >(column::branch_bit)] = node.source->branch_bit;
                if (node.source->value)
                {
                    local_index local = node.source->value->first;
                    slot_state const slot = storage_state(local, node.source->value->second);
                    record[static_cast< std::size_t >(column::lifetime_flags)] = static_cast< std::uint64_t >(slot.alive()) |
                        (slot.retained_storage && slot.storage_valid ? 2 : 0) | (slot.construction_pending && slot.storage_valid ? 4 : 0);
                    if (slot.delegate_of.has_value()) record[static_cast< std::size_t >(column::owner)] = static_cast< std::uint64_t >(*slot.delegate_of) + 1;
                    else if (slot.array_delegate_of_initializer.has_value()) record[static_cast< std::size_t >(column::owner)] = static_cast< std::uint64_t >(*slot.array_delegate_of_initializer) + 1;
                    record[static_cast< std::size_t >(column::owns_lifetime)] = slot.owns_subobject_lifetime;
                    std::array kinds{lifetime_transition_kind::control_flow, lifetime_transition_kind::normal_exit,
                        lifetime_transition_kind::exceptional_exit, lifetime_transition_kind::instruction, lifetime_transition_kind::retained_exit};
                    for (std::size_t index = 0; index < kinds.size(); ++index)
                        record[static_cast< std::size_t >(column::control_flow_actions) + index] = possible_actions(local, slot, kinds[index]);
                }
                result.push_back(record);
            }
            return result;
        }

        /** Selects the next mandatory action without materializing a cleanup list. */
        std::optional< selected_action > next(cursor& progress) const
        {
            if (progress.completed) return std::nullopt;
            if (progress.pending_end.has_value())
            {
                node_index version = *progress.pending_end;
                progress.pending_end.reset();
                return selected_action{{lifetime_action_kind::end_lifetime, leaf(version).first}, version};
            }
            transition const& selected = transitions_.at(progress.transition_index);
            while (true)
            {
                std::optional< node_index > version = predecessor(selected.source, progress.before);
                if (!version.has_value())
                {
                    if (progress.publication)
                    {
                        progress.completed = true;
                        return std::nullopt;
                    }
                    if (progress.ordinary) progress.publication = true;
                    else progress.ordinary = true;
                    progress.before.reset();
                    continue;
                }
                state_map::value_type const& entry = leaf(*version);
                progress.before = static_cast< std::uint64_t >(entry.first);
                slot_state const slot = storage_state(entry.first, entry.second);
                slot_state const* destination = lookup(selected.target, entry.first);
                std::optional< slot_state > retained_destination;
                if (destination != nullptr)
                {
                    retained_destination = storage_state(entry.first, *destination);
                    destination = &*retained_destination;
                }
                if (destination != nullptr && (destination->alive() ||
                    (slot.construction_pending && destination->construction_pending && destination->storage_valid) ||
                    (!slot.alive() && destination->retained_storage && destination->storage_valid))) continue;
                if (slot.persistent_storage || (selected.kind == lifetime_transition_kind::retained_exit && slot.retained_storage)) continue;
                if (!progress.ordinary)
                {
                    if (selected.kind != lifetime_transition_kind::instruction && slot.delegate_of.has_value() && slot.struct_delegate_selector.has_value() &&
                        !slot.destruction_started && slot.dtor_enabled() && slot.nontrivial_dtor.has_value() && !owner_survives(selected, *slot.delegate_of))
                    {
                        return selected_action{{lifetime_action_kind::destroy_object, entry.first}, *version};
                    }
                    continue;
                }
                if (!slot.alive() && !(slot.retained_storage && slot.storage_valid)) continue;
                type_symbol const& type = routine_.local_types.at(entry.first).type;
                bool alias = !slot.owns_subobject_lifetime && (slot.delegate_of.has_value() || slot.array_delegate_of_initializer.has_value() || slot.destroy_delegate || slot.is_projection);
                bool publication = !alias && slot.lifecycle_phase.has_value() && slot.lifecycle_phase->publish_on_normal_exit &&
                    selected.kind == lifetime_transition_kind::normal_exit;
                if (progress.publication != publication) continue;
                std::optional< lifetime_action_kind > action;
                bool completed = false;
                if (selected.kind != lifetime_transition_kind::instruction && type.type_is< array_initializer_type >())
                {
                    action = lifetime_action_kind::destroy_array_prefix;
                }
                else if (!alias && selected.kind != lifetime_transition_kind::instruction)
                {
                    bool preserves_parameter = (selected.kind == lifetime_transition_kind::normal_exit || selected.kind == lifetime_transition_kind::retained_exit ||
                        selected.kind == lifetime_transition_kind::exceptional_exit) && destroy_parameters_.contains(entry.first);
                    if (slot.lifecycle_phase.has_value())
                    {
                        action = lifetime_action_kind::restore_phase;
                        completed = slot.lifecycle_phase->publish_on_normal_exit && selected.kind == lifetime_transition_kind::normal_exit;
                    }
                    else if (type.type_is< initguard_lock_type >()) action = lifetime_action_kind::abort_initguard;
                    else if (!slot.destruction_started && slot.dtor_enabled() && routine_.asynchronous_cleanup_slots.contains(entry.first) && !preserves_parameter)
                    {
                        action = lifetime_action_kind::require_asynchronous_cleanup;
                    }
                    else if (!slot.destruction_started && slot.dtor_enabled() && (slot.nontrivial_dtor.has_value() || routine_.non_trivial_dtors.contains(type)) && !preserves_parameter)
                    {
                        action = lifetime_action_kind::destroy_object;
                    }
                }
                if (action.has_value())
                {
                    if (!alias) progress.pending_end = *version;
                    return selected_action{{*action, entry.first, completed}, *version};
                }
                if (!alias) return selected_action{{lifetime_action_kind::end_lifetime, entry.first}, *version};
            }
        }

      private:
        functanoid_routine3 const& routine_;
        snapshot_graph graph_;
        std::vector< transition > transitions_;
        std::vector< bool > source_nodes_;
        std::vector< node_index > source_leaves_;
        std::set< local_index > destroy_parameters_;

        /** Encodes actions whose remaining conditions depend on the runtime destination. */
        std::uint64_t possible_actions(local_index local, slot_state const& slot, lifetime_transition_kind kind) const
        {
            bool captured_alias = slot.storage_valid && !routine_.local_types.at(local).type.type_is< array_initializer_type >() &&
                (slot.is_projection || slot.delegate_of.has_value() || slot.array_delegate_of_initializer.has_value() || slot.storage_origin.has_value());
            std::uint64_t actions = captured_alias ? release_alias : 0;
            if (slot.persistent_storage) return 0;
            if (kind == lifetime_transition_kind::retained_exit && slot.retained_storage) return actions;
            if (kind != lifetime_transition_kind::instruction && slot.delegate_of.has_value() && slot.struct_delegate_selector.has_value() &&
                !slot.destruction_started && slot.dtor_enabled() && slot.nontrivial_dtor.has_value()) actions |= delegate_action;
            if (!slot.alive() && !(slot.retained_storage && slot.storage_valid)) return actions;
            type_symbol const& type = routine_.local_types.at(local).type;
            bool alias = !slot.owns_subobject_lifetime && (slot.delegate_of.has_value() || slot.array_delegate_of_initializer.has_value() || slot.destroy_delegate || slot.is_projection);
            auto append = [&](lifetime_action_kind action) { actions |= std::uint64_t{1} << static_cast< std::uint64_t >(action); };
            if (kind != lifetime_transition_kind::instruction && type.type_is< array_initializer_type >()) append(lifetime_action_kind::destroy_array_prefix);
            else if (!alias && kind != lifetime_transition_kind::instruction)
            {
                bool preserves_parameter = (kind == lifetime_transition_kind::normal_exit || kind == lifetime_transition_kind::retained_exit ||
                    kind == lifetime_transition_kind::exceptional_exit) && destroy_parameters_.contains(local);
                if (slot.lifecycle_phase.has_value())
                {
                    if (slot.lifecycle_phase->publish_on_normal_exit && kind == lifetime_transition_kind::normal_exit)
                        return actions | publication_action;
                    append(lifetime_action_kind::restore_phase);
                }
                else if (type.type_is< initguard_lock_type >()) append(lifetime_action_kind::abort_initguard);
                else if (!slot.destruction_started && slot.dtor_enabled() && routine_.asynchronous_cleanup_slots.contains(local) && !preserves_parameter)
                    append(lifetime_action_kind::require_asynchronous_cleanup);
                else if (!slot.destruction_started && slot.dtor_enabled() && (slot.nontrivial_dtor.has_value() || routine_.non_trivial_dtors.contains(type)) && !preserves_parameter)
                    append(lifetime_action_kind::destroy_object);
            }
            if (!alias) append(lifetime_action_kind::end_lifetime);
            return actions;
        }

        /** Propagates source reachability once through each shared node. */
        void mark_source(std::optional< node_index > root)
        {
            if (!root.has_value() || source_nodes_.at(*root)) return;
            source_nodes_.at(*root) = true;
            snapshot_graph::node const& node = graph_.nodes().at(*root);
            if (node.source->value) source_leaves_.push_back(*root);
            mark_source(node.left);
            mark_source(node.right);
        }

        /** Includes allocated frame reservations introduced by parked ownership. */
        slot_state storage_state(local_index local, slot_state state) const
        {
            if (routine_.coroutine.has_value() && routine_.coroutine->cleanup_retained.contains(local)) state.retained_storage = true;
            return state;
        }

        /** Returns the typed payload of one indexed leaf. */
        state_map::value_type const& leaf(node_index index) const { return *graph_.nodes().at(index).source->value; }

        /** Finds one slot version using at most one branch per local-index bit. */
        slot_state const* lookup(std::optional< node_index > root, local_index key) const
        {
            if (!root.has_value()) return nullptr;
            node_index current = *root;
            while (true)
            {
                snapshot_graph::node const& node = graph_.nodes().at(current);
                if (node.source->value)
                {
                    state_map::value_type const& value = *node.source->value;
                    return value.first == key ? &value.second : nullptr;
                }
                current = (static_cast< std::uint64_t >(key) & node.source->branch_bit) == 0 ? *node.left : *node.right;
            }
        }

        /** Finds the greatest source key below a cursor without retaining a traversal stack. */
        std::optional< node_index > predecessor(std::optional< node_index > root, std::optional< std::uint64_t > before) const
        {
            if (!root.has_value()) return std::nullopt;
            node_index current = *root;
            if (before.has_value() && graph_.nodes().at(current).source->minimum_key >= *before) return std::nullopt;
            while (true)
            {
                snapshot_graph::node const& node = graph_.nodes().at(current);
                if (node.source->value) return current;
                bool right = !before.has_value() || graph_.nodes().at(*node.right).source->minimum_key < *before;
                current = right ? *node.right : *node.left;
            }
        }

        /** Resolves ownership against the selected snapshots while diagnosing cycles. */
        bool owner_survives(transition const& selected, local_index owner) const
        {
            std::size_t remaining = selected.source.has_value() ? graph_.nodes().at(*selected.source).source->count : 0;
            while (true)
            {
                slot_state const* destination = lookup(selected.target, owner);
                if (destination != nullptr && destination->alive()) return true;
                slot_state const* source = lookup(selected.source, owner);
                if (source == nullptr || source->owns_subobject_lifetime) return false;
                if (remaining == 0) throw invalid_instruction_transition_error("Cyclic subobject ownership in lifetime transition");
                --remaining;
                if (source->delegate_of.has_value()) owner = *source->delegate_of;
                else if (source->array_delegate_of_initializer.has_value()) owner = *source->array_delegate_of_initializer;
                else return false;
            }
        }
    };
}

#endif // QUXLANG_VMIR2_LIFETIME_CLEANUP_PROGRAM_HEADER_GUARD
