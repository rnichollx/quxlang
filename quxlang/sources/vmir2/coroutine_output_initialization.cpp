// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/vmir2/coroutine_output_initialization.hpp>

#include <quxlang/vmir2/lifetime_snapshot_graph.hpp>

#include <set>
#include <utility>

quxlang::vmir2::coroutine_output_initialization::coroutine_output_initialization(
    functanoid_routine3 const& routine, std::map< local_index, type_symbol > const& borrowed_parameters)
    : roots_(routine.local_types.size()), members_(routine.local_types.size()), owners_(routine.local_types.size())
{
    std::vector< local_index > pending;
    for (std::pair< local_index const, type_symbol > const& parameter : borrowed_parameters)
    {
        if (!parameter.second.type_is< nvalue_slot >()) continue;
        roots_.at(parameter.first) = true;
        members_.at(parameter.first) = true;
        pending.push_back(parameter.first);
        ++root_count_;
    }
    if (root_count_ == 0) return;
    std::vector< std::vector< local_index > > children(routine.local_types.size());
    std::vector< bool > ambiguous(routine.local_types.size());
    std::map< std::pair< local_index, struct_init_selector >, local_index > selectors;
    std::map< std::vector< struct_init_delegate > const*, std::set< local_index > > payloads;
    auto register_delegates = [&](local_index parent, std::vector< struct_init_delegate > const& delegates)
    {
        if (!payloads[&delegates].insert(parent).second) return;
        for (struct_init_delegate const& delegate : delegates)
        {
            ++ownership_visits_;
            std::optional< owner >& prior = owners_.at(delegate.value);
            if (prior.has_value() && (prior->parent != parent || prior->selector != delegate.selector)) ambiguous.at(delegate.value) = true;
            else prior = owner{.parent = parent, .selector = delegate.selector};
            std::pair< std::map< std::pair< local_index, struct_init_selector >, local_index >::iterator, bool > selected = selectors.emplace(
                std::pair{parent, delegate.selector}, delegate.value);
            if (!selected.second && selected.first->second != delegate.value)
            {
                ambiguous.at(delegate.value) = true;
                ambiguous.at(selected.first->second) = true;
            }
            children.at(parent).push_back(delegate.value);
        }
    };
    lifetime_snapshot_graph< state_map > graph;
    for (executable_block const& block : routine.blocks)
    {
        graph.append(block.entry_state.snapshot_root());
        for (vm_instruction const& instruction : block.instructions)
        {
            if (instruction.type_is< struct_init_start >())
            {
                struct_init_start const& start = instruction.as< struct_init_start >();
                register_delegates(start.on_value, start.delegates);
            }
        }
    }
    for (lifetime_snapshot_graph< state_map >::node const& node : graph.nodes())
    {
        if (node.source->value && node.source->value->second.struct_delegates.has_value())
        {
            register_delegates(node.source->value->first, *node.source->value->second.struct_delegates);
        }
    }
    for (std::size_t index = 0; index < pending.size(); ++index)
    {
        local_index parent = pending[index];
        if (ambiguous.at(parent)) valid_ = false;
        for (local_index child : children.at(parent))
        {
            if (roots_.at(child)) valid_ = false;
            if (!members_.at(child))
            {
                members_[child] = true;
                pending.push_back(child);
            }
        }
    }
}

auto quxlang::vmir2::coroutine_output_initialization::summarize(state_map::snapshot root) -> obligations
{
    if (!root) return {};
    std::map< state_map::snapshot_node const*, cached_obligations >::const_iterator previous = summaries_.find(root.get());
    if (previous != summaries_.end()) return previous->second.counts;
    obligations result;
    if (root->value)
    {
        local_index local = root->value->first;
        slot_state const& slot = root->value->second;
        if (local < members_.size() && members_.at(local) && slot.storage_valid)
        {
            if (roots_.at(local) && slot.alive()) result.present_roots = 1;
            if (slot.construction_pending) result.unknown = true;
            else if (!roots_.at(local) && (!slot.delegate_of.has_value() || !slot.struct_delegate_selector.has_value() || !owners_.at(local).has_value() ||
                *slot.delegate_of != owners_[local]->parent || *slot.struct_delegate_selector != owners_[local]->selector))
            {
                result.unknown = true;
            }
            else if (slot.stage == slot_stage::full) result.completed = 1;
            else if (slot.stage == slot_stage::partial && slot.struct_delegates.has_value())
            {
                result.required = slot.struct_delegates->size();
                result.completed = 1;
            }
        }
    }
    else
    {
        obligations left = summarize(root->left);
        obligations right = summarize(root->right);
        result = {
            .required = left.required + right.required,
            .completed = left.completed + right.completed,
            .present_roots = left.present_roots + right.present_roots,
            .unknown = left.unknown || right.unknown,
        };
    }
    summaries_.emplace(root.get(), cached_obligations{.source = std::move(root), .counts = result});
    return result;
}

bool quxlang::vmir2::coroutine_output_initialization::completed(state_map const& state)
{
    if (root_count_ == 0) return true;
    if (!valid_) return false;
    obligations counts = summarize(state.snapshot_root());
    return !counts.unknown && counts.present_roots == root_count_ && counts.required + root_count_ == counts.completed;
}
