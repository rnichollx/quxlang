// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/vmir2/coroutine_frame_analysis.hpp>

#include "coroutine_frame_analysis_internal.hpp"

#include <algorithm>
#include <bit>
#include <functional>
#include <limits>
#include <map>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

std::uint64_t quxlang::vmir2::coroutine_refinement_work_limit(std::uint64_t input_size)
{
    if (input_size == 0) return 0;
    std::uint64_t multiplier = 64 * static_cast< std::uint64_t >(std::bit_width(input_size));
    return input_size > std::numeric_limits< std::uint64_t >::max() / multiplier ?
        std::numeric_limits< std::uint64_t >::max() : input_size * multiplier;
}

auto quxlang::vmir2::analyze_coroutine_frame(coroutine_frame_input const& input) -> coroutine_frame_analysis
{
    coroutine_frame_analysis output;
    output.component_ranks.resize(input.nodes.size());
    output.objects.resize(input.objects.size());
    output.alignment = input.header_alignment;

    auto valid_alignment = [](std::uint64_t alignment) -> bool
    {
        return alignment != 0 && std::has_single_bit(alignment);
    };
    if (!valid_alignment(input.header_alignment))
    {
        throw std::invalid_argument("Coroutine frame header alignment must be a nonzero power of two");
    }
    if (!input.nodes.empty() && input.entry_node >= input.nodes.size())
    {
        throw std::invalid_argument("Coroutine entry node is outside the logical CFG");
    }

    auto add_input_size = [&](std::size_t count)
    {
        if (count > std::numeric_limits< std::uint64_t >::max() - output.statistics.input_size)
        {
            throw std::overflow_error("Coroutine analysis input exceeds its work-count representation");
        }
        output.statistics.input_size += count;
    };
    add_input_size(1);
    add_input_size(input.nodes.size());
    add_input_size(input.objects.size());
    std::vector< std::vector< std::size_t > > predecessors(input.nodes.size());
    for (std::size_t node = 0; node < input.nodes.size(); ++node)
    {
        add_input_size(input.nodes[node].successors.size());
        for (std::size_t successor : input.nodes[node].successors)
        {
            ++output.statistics.graph_visits;
            if (successor >= input.nodes.size())
            {
                throw std::invalid_argument("Coroutine CFG successor is outside the logical CFG");
            }
            predecessors[successor].push_back(node);
        }
    }
    for (coroutine_frame_object const& object : input.objects)
    {
        if (!valid_alignment(object.alignment))
        {
            throw std::invalid_argument("Coroutine object alignment must be a nonzero power of two");
        }
        add_input_size(object.events.size());
        for (coroutine_lifetime_event const& event : object.events)
        {
            ++output.statistics.lifetime_visits;
            if (event.node >= input.nodes.size())
            {
                throw std::invalid_argument("Coroutine lifetime event is outside the logical CFG");
            }
        }
    }

    // Iterative Kosaraju traversal avoids compiler stack growth for deeply nested CFGs.
    std::vector< bool > reachable(input.nodes.size(), false);
    std::vector< std::pair< std::size_t, std::size_t > > traversal;
    std::vector< std::size_t > finish_order;
    if (!input.nodes.empty())
    {
        reachable[input.entry_node] = true;
        traversal.emplace_back(input.entry_node, 0);
    }
    while (!traversal.empty())
    {
        std::pair< std::size_t, std::size_t >& current = traversal.back();
        std::vector< std::size_t > const& successors = input.nodes[current.first].successors;
        ++output.statistics.graph_visits;
        if (current.second == successors.size())
        {
            finish_order.push_back(current.first);
            traversal.pop_back();
            continue;
        }
        std::size_t successor = successors[current.second++];
        if (!reachable[successor])
        {
            reachable[successor] = true;
            traversal.emplace_back(successor, 0);
        }
    }

    std::vector< std::optional< std::size_t > > component(input.nodes.size());
    std::vector< std::vector< std::size_t > > component_nodes;
    std::vector< std::size_t > pending;
    for (std::vector< std::size_t >::const_reverse_iterator entry = finish_order.crbegin(); entry != finish_order.crend(); ++entry)
    {
        if (component[*entry].has_value())
        {
            continue;
        }
        std::size_t ordinal = component_nodes.size();
        component_nodes.emplace_back();
        component[*entry] = ordinal;
        pending.push_back(*entry);
        while (!pending.empty())
        {
            std::size_t node = pending.back();
            pending.pop_back();
            component_nodes[ordinal].push_back(node);
            ++output.statistics.graph_visits;
            for (std::size_t predecessor : predecessors[node])
            {
                ++output.statistics.graph_visits;
                if (reachable[predecessor] && !component[predecessor].has_value())
                {
                    component[predecessor] = ordinal;
                    pending.push_back(predecessor);
                }
            }
        }
    }

    // Kosaraju discovers components in topological order; parallel edges need no deduplication.
    output.statistics.component_count = component_nodes.size();
    std::vector< std::size_t > suspension_prefix(component_nodes.size() + 1, 0);
    for (std::size_t ordinal = 0; ordinal < component_nodes.size(); ++ordinal)
    {
        bool may_suspend = false;
        for (std::size_t node : component_nodes[ordinal])
        {
            ++output.statistics.graph_visits;
            output.component_ranks[node] = ordinal;
            may_suspend = may_suspend || input.nodes[node].may_suspend;
            for (std::size_t successor : input.nodes[node].successors)
            {
                ++output.statistics.graph_visits;
                if (!component[successor].has_value() || *component[successor] < ordinal)
                {
                    throw std::invalid_argument("Coroutine SCC traversal did not produce topological components");
                }
            }
        }
        suspension_prefix[ordinal + 1] = suspension_prefix[ordinal] + static_cast< std::size_t >(may_suspend);
    }

    std::vector< std::size_t > persistent;
    for (std::size_t index = 0; index < input.objects.size(); ++index)
    {
        coroutine_frame_object const& object = input.objects[index];
        coroutine_object_placement& placement = output.objects[index];
        std::optional< std::size_t > first_rank;
        std::size_t last_rank = 0;
        std::optional< std::size_t > sole_node;
        bool multiple_nodes = false;
        std::size_t begin_count = 0;
        std::size_t end_count = 0;
        std::size_t begin_instruction = 0;
        std::size_t end_instruction = 0;
        std::optional< std::size_t > first_instruction;
        std::size_t last_instruction = 0;
        for (coroutine_lifetime_event const& event : object.events)
        {
            ++output.statistics.lifetime_visits;
            if (!component[event.node].has_value())
            {
                continue;
            }
            std::size_t rank = *component[event.node];
            first_rank = first_rank.has_value() ? std::min(*first_rank, rank) : rank;
            last_rank = std::max(last_rank, rank);
            if (!sole_node.has_value())
            {
                sole_node = event.node;
            }
            else if (*sole_node != event.node)
            {
                multiple_nodes = true;
            }
            first_instruction = first_instruction.has_value() ? std::min(*first_instruction, event.instruction_index) : event.instruction_index;
            last_instruction = std::max(last_instruction, event.instruction_index);
            if (event.kind == coroutine_lifetime_event_kind::begin)
            {
                ++begin_count;
                begin_instruction = event.instruction_index;
            }
            else if (event.kind == coroutine_lifetime_event_kind::end)
            {
                ++end_count;
                end_instruction = event.instruction_index;
            }
        }

        placement.reachable = first_rank.has_value() || object.force_persistent || !object.lifetime_complete;
        if (!placement.reachable)
        {
            continue;
        }
        if (!object.lifetime_complete || !first_rank.has_value())
        {
            placement.first_component = 0;
            placement.last_component = component_nodes.empty() ? 0 : component_nodes.size() - 1;
        }
        else
        {
            placement.first_component = *first_rank;
            placement.last_component = last_rank;
        }

        bool crosses_suspension = !component_nodes.empty() &&
            suspension_prefix[placement.last_component + 1] != suspension_prefix[placement.first_component];
        bool single_activation = object.lifetime_complete && sole_node.has_value() && !multiple_nodes &&
            !input.nodes[*sole_node].may_suspend && begin_count == 1 && end_count == 1 &&
            begin_instruction < end_instruction && begin_instruction == *first_instruction && end_instruction == last_instruction;
        if (!object.force_persistent && (!crosses_suspension || single_activation))
        {
            continue;
        }
        if (!object.force_persistent && object.rematerializable && !object.requires_stable_address)
        {
            placement.kind = coroutine_storage_kind::rematerializable;
            continue;
        }
        placement.kind = object.relocatable && !object.requires_stable_address ? coroutine_storage_kind::spillable : coroutine_storage_kind::stable_address;
        output.alignment = std::max(output.alignment, object.alignment);
        persistent.push_back(index);
    }
    output.statistics.persistent_objects = persistent.size();

    auto aligned_offset = [&](std::uint64_t offset, std::uint64_t alignment) -> std::uint64_t
    {
        std::uint64_t padding = (alignment - (offset & (alignment - 1))) & (alignment - 1);
        if (offset > input.maximum_frame_size || padding > input.maximum_frame_size - offset)
        {
            throw std::overflow_error("Coroutine frame alignment exceeds the target storage limit");
        }
        return offset + padding;
    };
    auto advance_extent = [&](std::uint64_t offset, std::uint64_t size) -> std::uint64_t
    {
        if (offset > input.maximum_frame_size || size > input.maximum_frame_size - offset)
        {
            throw std::overflow_error("Coroutine frame size exceeds the target storage limit");
        }
        return offset + size;
    };

    std::sort(persistent.begin(), persistent.end(), [&](std::size_t lhs, std::size_t rhs)
    {
        coroutine_object_placement const& left = output.objects[lhs];
        coroutine_object_placement const& right = output.objects[rhs];
        if (left.first_component != right.first_component) return left.first_component < right.first_component;
        if (left.last_component != right.last_component) return left.last_component < right.last_component;
        return lhs < rhs;
    });

    using cell_queue = std::priority_queue< std::size_t, std::vector< std::size_t >, std::greater< std::size_t > >;
    using active_cell = std::pair< std::size_t, std::size_t >;
    std::priority_queue< active_cell, std::vector< active_cell >, std::greater< active_cell > > active;
    std::map< std::pair< std::size_t, std::uint64_t >, cell_queue > reusable;
    std::vector< coroutine_frame_cell > cells;
    std::vector< std::optional< std::size_t > > object_cells(input.objects.size());
    std::uint64_t minimum_extent = input.header_size;
    for (std::size_t index : persistent)
    {
        coroutine_object_placement const& placement = output.objects[index];
        coroutine_frame_object const& object = input.objects[index];
        minimum_extent = std::max(minimum_extent, advance_extent(aligned_offset(input.header_size, object.alignment), object.size));
        while (!active.empty() && active.top().first < placement.first_component)
        {
            std::size_t cell = active.top().second;
            active.pop();
            reusable[{cells[cell].size_class, cells[cell].alignment}].push(cell);
        }
        std::size_t size_class = static_cast< std::size_t >(std::bit_width(object.size == 0 ? std::uint64_t{0} : object.size - 1));
        cell_queue& available = reusable[{size_class, object.alignment}];
        std::size_t cell;
        if (available.empty())
        {
            cell = cells.size();
            cells.push_back({.size = object.size, .alignment = object.alignment, .size_class = size_class});
        }
        else
        {
            cell = available.top();
            available.pop();
            cells[cell].size = std::max(cells[cell].size, object.size);
        }
        object_cells[index] = cell;
        active.emplace(placement.last_component, cell);
    }
    std::vector< std::size_t > cell_order(cells.size());
    std::iota(cell_order.begin(), cell_order.end(), 0);
    std::sort(cell_order.begin(), cell_order.end(), [&](std::size_t lhs, std::size_t rhs)
    {
        if (cells[lhs].alignment != cells[rhs].alignment) return cells[lhs].alignment > cells[rhs].alignment;
        if (cells[lhs].size != cells[rhs].size) return cells[lhs].size > cells[rhs].size;
        return lhs < rhs;
    });
    std::uint64_t extent = advance_extent(0, input.header_size);
    for (std::size_t cell : cell_order)
    {
        cells[cell].offset = aligned_offset(extent, cells[cell].alignment);
        extent = advance_extent(cells[cell].offset, cells[cell].size);
    }
    output.size = aligned_offset(extent, output.alignment);
    for (std::size_t index : persistent)
    {
        output.objects[index].offset = cells[*object_cells[index]].offset;
    }
    output.statistics.baseline_cells = cells.size();
    output.statistics.baseline_size = output.size;

    std::uint64_t automatic_limit = coroutine_refinement_work_limit(output.statistics.input_size);
    output.statistics.refinement_limit = input.refinement_work_limit.has_value() ? std::min(automatic_limit, *input.refinement_work_limit) : automatic_limit;
    // Every layout must accommodate each individual object after the header.
    if (output.size == aligned_offset(minimum_extent, output.alignment)) return output;
    auto spend = [&](std::uint64_t amount) -> bool
    {
        if (amount > output.statistics.refinement_limit - output.statistics.refinement_work)
        {
            output.statistics.refinement_exhausted = true;
            return false;
        }
        output.statistics.refinement_work += amount;
        return true;
    };
    if (persistent.size() < 2 || !spend(persistent.size()))
    {
        return output;
    }

    // Bottom-up merge sorting can stop at any comparison or copy without an unchecked sort.
    auto budgeted_sort = [&]< typename Value, typename Compare >(std::vector< Value >& values, Compare compare) -> bool
    {
        if (values.size() < 2) return true;
        if (!spend(values.size())) return false;
        std::vector< Value > scratch(values.size());
        for (std::size_t width = 1; width < values.size();)
        {
            for (std::size_t start = 0; start < values.size();)
            {
                std::size_t middle = start + std::min(width, values.size() - start);
                std::size_t finish = middle + std::min(width, values.size() - middle);
                std::size_t left = start;
                std::size_t right = middle;
                for (std::size_t position = start; position < finish; ++position)
                {
                    if (!spend(1)) return false;
                    bool choose_left = right == finish;
                    if (left < middle && right < finish)
                    {
                        if (!spend(1)) return false;
                        choose_left = !compare(values[right], values[left]);
                    }
                    scratch[position] = choose_left ? values[left++] : values[right++];
                }
                start = finish;
            }
            values.swap(scratch);
            if (width > values.size() / 2) break;
            width *= 2;
        }
        return true;
    };

    std::vector< std::size_t > candidate_order = persistent;
    if (!budgeted_sort(candidate_order, [&](std::size_t lhs, std::size_t rhs)
    {
        if (input.objects[lhs].alignment != input.objects[rhs].alignment) return input.objects[lhs].alignment > input.objects[rhs].alignment;
        if (input.objects[lhs].size != input.objects[rhs].size) return input.objects[lhs].size > input.objects[rhs].size;
        return lhs < rhs;
    }))
    {
        return output;
    }
    if (!spend(input.objects.size())) return output;
    std::vector< std::uint64_t > candidate_offsets(input.objects.size(), 0);
    std::vector< std::pair< std::uint64_t, std::uint64_t > > forbidden;
    if (!spend(persistent.size())) return output;
    forbidden.reserve(persistent.size());
    std::uint64_t candidate_extent = input.header_size;
    std::size_t completed = 0;
    try
    {
        for (; completed < candidate_order.size(); ++completed)
        {
            std::size_t index = candidate_order[completed];
            coroutine_frame_object const& object = input.objects[index];
            coroutine_object_placement const& placement = output.objects[index];
            forbidden.clear();
            bool complete_proof = true;
            for (std::size_t previous = 0; previous < completed; ++previous)
            {
                if (!spend(1))
                {
                    complete_proof = false;
                    break;
                }
                std::size_t other_index = candidate_order[previous];
                coroutine_object_placement const& other = output.objects[other_index];
                if (placement.last_component < other.first_component || other.last_component < placement.first_component)
                {
                    continue;
                }
                forbidden.emplace_back(candidate_offsets[other_index], candidate_offsets[other_index] + input.objects[other_index].size);
            }
            if (!complete_proof || !budgeted_sort(forbidden, std::less< std::pair< std::uint64_t, std::uint64_t > >{})) break;
            std::uint64_t offset = aligned_offset(input.header_size, object.alignment);
            for (std::pair< std::uint64_t, std::uint64_t > const& range : forbidden)
            {
                if (!spend(1))
                {
                    complete_proof = false;
                    break;
                }
                if (range.second <= offset || object.size == 0) continue;
                if (offset <= range.first && object.size <= range.first - offset) break;
                offset = aligned_offset(range.second, object.alignment);
            }
            if (!complete_proof) break;
            candidate_extent = std::max(candidate_extent, advance_extent(offset, object.size));
            candidate_offsets[index] = offset;
        }

        // The remaining objects receive a disjoint tail; no unfinished overlap proof is used.
        for (; completed < candidate_order.size(); ++completed)
        {
            std::size_t index = candidate_order[completed];
            candidate_offsets[index] = aligned_offset(candidate_extent, input.objects[index].alignment);
            candidate_extent = advance_extent(candidate_offsets[index], input.objects[index].size);
            if (candidate_extent > output.size) return output;
        }
        std::uint64_t candidate_size = aligned_offset(candidate_extent, output.alignment);
        if (candidate_size < output.size)
        {
            output.size = candidate_size;
            output.statistics.refined_layout_selected = true;
            for (std::size_t index : persistent)
            {
                output.objects[index].offset = candidate_offsets[index];
            }
        }
    }
    catch (std::overflow_error const&)
    {
        // An optional placement cannot reject a representable conservative layout.
    }
    return output;
}
