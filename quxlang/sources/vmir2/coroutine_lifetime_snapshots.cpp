// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "coroutine_lifetime_snapshots.hpp"

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/vmir2/lifetime_snapshot_graph.hpp>
#include <quxlang/vmir2/local_operands.hpp>

#include <map>
#include <utility>

namespace quxlang::vmir2
{
    /** Joins obligations for immutable metadata while preserving discovery order. */
    template < typename Value >
    class shared_lifetime_obligations
    {
      public:
        /** Adds the range of one reference to a shared metadata payload. */
        void include(Value const* value, lifetime_snapshot_range const& range)
        {
            std::pair< typename std::map< Value const*, std::size_t >::iterator, bool > entry = indices_.emplace(value, values_.size());
            if (entry.second) values_.push_back({value, range});
            else values_[entry.first->second].second.include(range);
        }

        /** Visits each shared payload with its complete conservative lifetime range. */
        std::vector< std::pair< Value const*, lifetime_snapshot_range > > const& values() const { return values_; }

      private:
        std::map< Value const*, std::size_t > indices_;
        std::vector< std::pair< Value const*, lifetime_snapshot_range > > values_;
    };
}

auto quxlang::vmir2::record_coroutine_snapshot_lifetimes(functanoid_routine3 const& routine, std::vector< coroutine_snapshot_observation > const& observations,
                                                       coroutine_vmir_analysis& output) -> coroutine_snapshot_statistics
{
    coroutine_snapshot_statistics statistics{.roots = observations.size()};
    coroutine_frame_input graph_input{.nodes = output.input.nodes, .entry_node = output.input.entry_node};
    coroutine_frame_analysis order = analyze_coroutine_frame(graph_input);
    lifetime_snapshot_graph< state_map > graph;
    std::vector< std::optional< std::size_t > > roots;
    roots.reserve(observations.size());
    for (coroutine_snapshot_observation const& observation : observations) roots.push_back(graph.append(observation.root));
    statistics.nodes = graph.nodes().size();
    std::vector< lifetime_snapshot_range > ranges(graph.nodes().size());
    for (std::size_t index = 0; index < observations.size(); ++index)
    {
        coroutine_snapshot_observation const& observation = observations[index];
        std::optional< std::size_t > component = order.component_ranks.at(observation.node);
        if (roots[index].has_value() && component.has_value())
        {
            ranges[*roots[index]].include({.component = *component, .node = observation.node, .instruction = observation.instruction});
        }
    }
    graph.propagate(ranges);
    auto record = [&](local_index local, lifetime_snapshot_range const& range)
    {
        ++statistics.metadata_operands;
        if (local == routine.coroutine->frame || local == routine.coroutine->controller || !range.first.has_value()) return;
        coroutine_frame_object& object = output.input.objects.at(static_cast< std::uint64_t >(local));
        object.events.push_back({.node = range.first->node, .instruction_index = range.first->instruction, .kind = coroutine_lifetime_event_kind::access});
        if (range.last != range.first)
        {
            object.events.push_back({.node = range.last->node, .instruction_index = range.last->instruction, .kind = coroutine_lifetime_event_kind::access});
        }
    };
    shared_lifetime_obligations< dtor_spec > destructors;
    shared_lifetime_obligations< std::vector< struct_init_delegate > > struct_delegates;
    shared_lifetime_obligations< invocation_args > array_delegates;
    using element_map = lifetime_state_map< local_index, bool >;
    lifetime_snapshot_graph< element_map > element_graph;
    std::vector< std::pair< std::size_t, lifetime_snapshot_range > > element_observations;
    for (std::size_t index = 0; index < graph.nodes().size(); ++index)
    {
        state_map::snapshot_node const& node = *graph.nodes()[index].source;
        if (!node.value || !ranges[index].first.has_value()) continue;
        ++statistics.slot_versions;
        slot_state const& slot = node.value->second;
        if (!slot.storage_valid && !slot.alive()) continue;
        local_index local = node.value->first;
        lifetime_snapshot_range const& range = ranges[index];
        record(local, range);
        visit_local_operands(slot.delegate_of, [&](local_index operand) { record(operand, range); });
        visit_local_operands(slot.storage_origin, [&](local_index operand) { record(operand, range); });
        visit_local_operands(slot.array_delegate_of_initializer, [&](local_index operand) { record(operand, range); });
        visit_local_operands(slot.lifecycle_phase, [&](local_index operand) { record(operand, range); });
        if (slot.nontrivial_dtor.has_value()) destructors.include(&*slot.nontrivial_dtor, range);
        if (slot.struct_delegates.has_value()) struct_delegates.include(&*slot.struct_delegates, range);
        if (slot.array_delegates.has_value()) array_delegates.include(&*slot.array_delegates, range);
        std::optional< std::size_t > elements = element_graph.append(slot.array_element_delegates.serialize_interface().snapshot_root());
        if (elements.has_value()) element_observations.push_back({*elements, range});
        if ((slot.is_projection || slot.delegate_of.has_value() || slot.array_delegate_of_initializer.has_value()) &&
            !routine.local_types.at(local).type.type_is< array_initializer_type >())
        {
            output.borrowed_locals.insert(local);
        }
        if (slot.array_delegate_of_initializer.has_value() && slot.stage == slot_stage::full && !slot.owns_subobject_lifetime)
        {
            output.saved_array_element_addresses.insert(local);
        }
    }
    auto record_payloads = [&]< typename Value >(shared_lifetime_obligations< Value > const& payloads)
    {
        statistics.metadata_payloads += payloads.values().size();
        for (std::pair< Value const*, lifetime_snapshot_range > const& payload : payloads.values())
        {
            visit_local_operands(*payload.first, [&](local_index local) { record(local, payload.second); });
        }
    };
    record_payloads(destructors);
    record_payloads(struct_delegates);
    record_payloads(array_delegates);
    std::vector< lifetime_snapshot_range > element_ranges(element_graph.nodes().size());
    for (std::pair< std::size_t, lifetime_snapshot_range > const& observation : element_observations)
    {
        element_ranges[observation.first].include(observation.second);
    }
    element_graph.propagate(element_ranges);
    statistics.nodes += element_graph.nodes().size();
    for (std::size_t index = 0; index < element_graph.nodes().size(); ++index)
    {
        element_map::snapshot_node const& node = *element_graph.nodes()[index].source;
        if (node.value) record(node.value->first, element_ranges[index]);
    }
    return statistics;
}
