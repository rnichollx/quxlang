// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_LIFETIME_TRANSITION_GRAPH_HEADER_GUARD
#define QUXLANG_VMIR2_LIFETIME_TRANSITION_GRAPH_HEADER_GUARD

#include <quxlang/vmir2/vmir2.hpp>
#include <quxlang/exception.hpp>

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <tuple>
#include <vector>

/** Selects the lifetime contract applied at a transition boundary. */
RPNX_ENUM(quxlang::vmir2, lifetime_transition_kind, std::uint8_t, control_flow, normal_exit, exceptional_exit, instruction, retained_exit);

/** Identifies one semantic cleanup operation in execution order. */
RPNX_ENUM(quxlang::vmir2, lifetime_action_kind, std::uint8_t, destroy_object, destroy_array_prefix, abort_initguard, restore_phase, require_asynchronous_cleanup, end_lifetime);

namespace quxlang::vmir2
{
    /** Associates a cleanup operation with its slot in the source state. */
    struct lifetime_action
    {
        lifetime_action_kind kind;
        local_index slot;
        /// Publishes successful completion when the action restores a lifecycle phase.
        bool completed = false;
    };

    /**
     * Shares ordered cleanup sequences between immutable lifetime snapshots.
     * Each target context memoizes source subtrees. Delegate ancestry retains
     * the complete source identity when its ownership proof reads other slots.
     * Normal-output completeness is validated by the calling lifetime engine.
     */
    class lifetime_transition_graph
    {
      public:
        using sequence = std::size_t;

        /** Concatenates two sequences or retains one semantic cleanup action. */
        struct node
        {
            sequence first = 0;
            sequence second = 0;
            std::optional< lifetime_action > action;
            state_map::snapshot source;
            bool redundant_end = false;
        };

        /** Orders delegated destruction, ordinary cleanup, and successful lifecycle publication. */
        struct plan
        {
            sequence delegates = 0;
            sequence ordinary = 0;
            sequence publication = 0;
            bool reads_ancestry = false;

            /** Reports whether the transition has a semantic cleanup action. */
            bool empty() const { return delegates == 0 && ordinary == 0 && publication == 0; }
        };

        /** Binds type and destructor information that remains valid as locals are appended. */
        explicit lifetime_transition_graph(functanoid_routine3 const& routine) : routine_(routine)
        {
            auto remember = [&](routine_parameter const& parameter)
            {
                if (parameter.type.type_is< dvalue_slot >()) destroy_parameters_.insert(parameter.local_index);
            };
            for (routine_parameter const& parameter : routine.parameters.positional) remember(parameter);
            for (std::pair< std::string const, routine_parameter > const& parameter : routine.parameters.named) remember(parameter.second);
        }

        /** Selects a shared cleanup plan without flattening its action sequences. */
        plan select(state_map const& source, state_map const& target, lifetime_transition_kind kind)
        {
            context_key key{target.snapshot_root().get(), kind};
            context& selected = contexts_[key];
            selected.target = target;
            selected.kind = kind;
            return visit(selected, source, source.snapshot_root());
        }

        /** Stops optional expansion before exceeding a deterministic shared work allowance. */
        std::optional< plan > select_bounded(state_map const& source, state_map const& target,
            lifetime_transition_kind kind, std::uint64_t& remaining)
        {
            remaining_work_ = &remaining;
            try
            {
                spend();
                plan result = select(source, target, kind);
                remaining_work_ = nullptr;
                return result;
            }
            catch (work_exhausted const&)
            {
                remaining_work_ = nullptr;
                return std::nullopt;
            }
            catch (...)
            {
                remaining_work_ = nullptr;
                throw;
            }
        }

        /** Returns one immutable action node; sequence zero denotes the empty sequence. */
        node const& at(sequence index) const { return nodes_.at(index - 1); }

        /** Counts materialized sequence nodes for deterministic work measurements. */
        std::size_t node_count() const { return nodes_.size(); }

        /** Counts previously unseen source/context pairs. */
        std::size_t snapshot_visits() const { return snapshot_visits_; }

        /** Produces the ordinary vector interface used by synchronous consumers. */
        std::vector< lifetime_action > flatten(plan selected) const
        {
            std::vector< lifetime_action > result;
            std::vector< sequence > pending{selected.publication, selected.ordinary, selected.delegates};
            while (!pending.empty())
            {
                sequence index = pending.back();
                pending.pop_back();
                if (index == 0) continue;
                node const& entry = at(index);
                if (entry.action.has_value()) result.push_back(*entry.action);
                else
                {
                    pending.push_back(entry.second);
                    pending.push_back(entry.first);
                }
            }
            return result;
        }

      private:
        using source_node = state_map::snapshot_node;
        using context_key = std::pair< source_node const*, lifetime_transition_kind >;
        using source_key = std::pair< source_node const*, source_node const* >;

        /** Pins source versions and the ordered sequences derived from them. */
        struct cached_plan
        {
            state_map::snapshot source;
            state_map::snapshot ancestry;
            plan value;
        };

        /** Holds the immutable destination and proofs belonging to one exit contract. */
        struct context
        {
            state_map target;
            lifetime_transition_kind kind = lifetime_transition_kind::control_flow;
            std::map< source_key, cached_plan > sources;
            std::map< std::pair< source_node const*, local_index >, std::optional< bool > > surviving_owners;
        };

        functanoid_routine3 const& routine_;
        std::set< local_index > destroy_parameters_;
        std::map< context_key, context > contexts_;
        std::vector< node > nodes_;
        std::size_t snapshot_visits_ = 0;
        std::uint64_t* remaining_work_ = nullptr;

        /** Identifies an unfinished optional proof whose cached complete subtrees remain valid. */
        struct work_exhausted {};

        /** Charges one represented operation before allocating or traversing its metadata. */
        void spend()
        {
            if (remaining_work_ == nullptr) return;
            if (*remaining_work_ == 0) throw work_exhausted{};
            --*remaining_work_;
        }

        /** Tests whether a destination owns a live slot. */
        static bool survives(state_map const& target, local_index slot)
        {
            state_map::const_iterator entry = target.find(slot);
            return entry != target.end() && entry->second.alive();
        }

        /** Memoizes ownership ancestry while preserving cycle diagnostics. */
        bool owner_survives(context& selected, state_map const& source, local_index owner)
        {
            std::vector< local_index > path;
            bool preserved = false;
            source_node const* root = source.snapshot_root().get();
            try
            {
                while (true)
                {
                    spend();
                    if (survives(selected.target, owner))
                    {
                        preserved = true;
                        break;
                    }
                    auto known = selected.surviving_owners.find({root, owner});
                    if (known != selected.surviving_owners.end())
                    {
                        if (!known->second.has_value()) throw invalid_instruction_transition_error("Cyclic subobject ownership in lifetime transition");
                        preserved = *known->second;
                        break;
                    }
                    selected.surviving_owners.emplace(std::pair{root, owner}, std::nullopt);
                    path.push_back(owner);
                    state_map::const_iterator entry = source.find(owner);
                    if (entry == source.end() || entry->second.owns_subobject_lifetime) break;
                    if (entry->second.delegate_of.has_value()) owner = *entry->second.delegate_of;
                    else if (entry->second.array_delegate_of_initializer.has_value()) owner = *entry->second.array_delegate_of_initializer;
                    else break;
                }
            }
            catch (...)
            {
                for (local_index index : path) selected.surviving_owners.erase({root, index});
                throw;
            }
            for (local_index index : path) selected.surviving_owners.at({root, index}) = preserved;
            return preserved;
        }

        /** Joins nonempty sequences without flattening either child. */
        sequence concatenate(sequence first, sequence second)
        {
            if (first == 0) return second;
            if (second == 0) return first;
            spend();
            nodes_.push_back({.first = first, .second = second});
            return nodes_.size();
        }

        /** Allocates a semantic action retaining the source slot version. */
        sequence append(lifetime_action_kind kind, state_map::snapshot source, bool redundant_end = false, bool completed = false)
        {
            spend();
            local_index slot = source->value->first;
            nodes_.push_back({.action = lifetime_action{kind, slot, completed}, .source = std::move(source), .redundant_end = redundant_end});
            return nodes_.size();
        }

        /** Classifies one departing object using the generic lifetime ownership rules. */
        plan classify(context& selected, state_map const& source, state_map::snapshot const& leaf)
        {
            local_index index = leaf->value->first;
            slot_state const& slot = leaf->value->second;
            state_map::const_iterator destination = selected.target.find(index);
            if (destination != selected.target.end() && (destination->second.alive() ||
                (slot.construction_pending && destination->second.construction_pending && destination->second.storage_valid) ||
                (!slot.alive() && destination->second.retained_storage && destination->second.storage_valid))) return {};
            if (slot.persistent_storage || (selected.kind == lifetime_transition_kind::retained_exit && slot.retained_storage)) return {};
            plan result;
            if (selected.kind != lifetime_transition_kind::instruction && slot.delegate_of.has_value() && slot.struct_delegate_selector.has_value() &&
                !slot.destruction_started && slot.dtor_enabled() && slot.nontrivial_dtor.has_value())
            {
                result.reads_ancestry = true;
                if (!owner_survives(selected, source, *slot.delegate_of)) result.delegates = append(lifetime_action_kind::destroy_object, leaf);
            }
            if (!slot.alive() && !(slot.retained_storage && slot.storage_valid)) return result;
            type_symbol const& type = routine_.local_types.at(index).type;
            bool alias = !slot.owns_subobject_lifetime && (slot.delegate_of.has_value() || slot.array_delegate_of_initializer.has_value() || slot.destroy_delegate || slot.is_projection);
            if (selected.kind != lifetime_transition_kind::instruction && type.type_is< array_initializer_type >())
            {
                result.ordinary = append(lifetime_action_kind::destroy_array_prefix, leaf);
            }
            else if (!alias && selected.kind != lifetime_transition_kind::instruction)
            {
                bool exit_preserves_parameter = (selected.kind == lifetime_transition_kind::normal_exit || selected.kind == lifetime_transition_kind::retained_exit ||
                    selected.kind == lifetime_transition_kind::exceptional_exit) && destroy_parameters_.contains(index);
                if (slot.lifecycle_phase.has_value())
                {
                    bool completed = slot.lifecycle_phase->publish_on_normal_exit && selected.kind == lifetime_transition_kind::normal_exit;
                    sequence action = append(lifetime_action_kind::restore_phase, leaf, false, completed);
                    if (completed) result.publication = action;
                    else result.ordinary = action;
                }
                else if (type.type_is< initguard_lock_type >()) result.ordinary = append(lifetime_action_kind::abort_initguard, leaf);
                else if (!slot.destruction_started && slot.dtor_enabled() && routine_.asynchronous_cleanup_slots.contains(index) && !exit_preserves_parameter)
                {
                    result.ordinary = append(lifetime_action_kind::require_asynchronous_cleanup, leaf);
                }
                else if (!slot.destruction_started && slot.dtor_enabled() && (slot.nontrivial_dtor.has_value() || routine_.non_trivial_dtors.contains(type)) && !exit_preserves_parameter)
                {
                    result.ordinary = append(lifetime_action_kind::destroy_object, leaf);
                }
            }
            if (!alias)
            {
                bool redundant = result.ordinary != 0 || result.delegates != 0 || result.publication != 0;
                sequence end = append(lifetime_action_kind::end_lifetime, leaf, redundant);
                if (result.publication != 0) result.publication = concatenate(result.publication, end);
                else result.ordinary = concatenate(result.ordinary, end);
            }
            return result;
        }

        /** Shares source subtrees within one destination context. */
        plan visit(context& selected, state_map const& source, state_map::snapshot root)
        {
            if (!root) return {};
            spend();
            source_node const* destination = selected.target.snapshot_root().get();
            while (destination != nullptr && !destination->value && destination->branch_bit > root->branch_bit)
            {
                spend();
                destination = (root->minimum_key & destination->branch_bit) == 0 ? destination->left.get() : destination->right.get();
            }
            if (destination == root.get()) return {};
            source_key key{root.get(), nullptr};
            auto previous = selected.sources.find(key);
            if (previous != selected.sources.end()) return previous->second.value;
            key.second = source.snapshot_root().get();
            previous = selected.sources.find(key);
            if (previous != selected.sources.end()) return previous->second.value;
            ++snapshot_visits_;
            plan result;
            if (root->value) result = classify(selected, source, root);
            else
            {
                plan lower = visit(selected, source, root->left);
                plan upper = visit(selected, source, root->right);
                result.delegates = concatenate(upper.delegates, lower.delegates);
                result.ordinary = concatenate(upper.ordinary, lower.ordinary);
                result.publication = concatenate(upper.publication, lower.publication);
                result.reads_ancestry = lower.reads_ancestry || upper.reads_ancestry;
            }
            state_map::snapshot ancestry = result.reads_ancestry ? source.snapshot_root() : state_map::snapshot{};
            key.second = ancestry.get();
            selected.sources.emplace(key, cached_plan{root, std::move(ancestry), result});
            return result;
        }
    };
}

#endif // QUXLANG_VMIR2_LIFETIME_TRANSITION_GRAPH_HEADER_GUARD
