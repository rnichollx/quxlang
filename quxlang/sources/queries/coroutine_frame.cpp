// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/coroutine_storage_dependencies.hpp>

#include <quxlang/data/coroutine_types.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/vmir2/coroutine_vmir_analysis.hpp>
#include <quxlang/vmir2/local_borrow_analysis.hpp>

#include <algorithm>
#include <stdexcept>
#include <set>
#include <vector>

auto quxlang::coroutine_storage_dependencies::co_add_captures(instanciation_reference const& coroutine) -> rpnx::querygraph::coroutine< coroutine_frame_spec >::cosubroutine< void >
{
    instatype const& parameters = co_await rpnx::querygraph::request< instanciation_concrete_params_query >(coroutine);
    auto append = [&](parameter_instantiation const& parameter)
    {
        std::optional< type_symbol > runtime = parameter_runtime_type(parameter_instantiation_type(parameter));
        if (!runtime.has_value() || is_ref(*runtime) || is_ptr(*runtime) ||
            runtime->type_is< nvalue_slot >() || runtime->type_is< dvalue_slot >()) return;
        pending.push_back(std::move(*runtime));
    };
    for (parameter_instantiation const& parameter : parameters.positional) append(parameter);
    for (std::pair< std::string const, parameter_instantiation > const& parameter : parameters.named)
    {
        if (parameter.first != "CONTROLLER") append(parameter.second);
    }
}

auto quxlang::coroutine_storage_dependencies::co_validate() -> rpnx::querygraph::coroutine< coroutine_frame_spec >::cosubroutine< void >
{
    while (!pending.empty())
    {
        type_symbol current = std::move(pending.back());
        pending.pop_back();
        if (current == frame)
        {
            throw semantic_compilation_error("Coroutine frame has a recursive storage dependency: " + to_string(frame));
        }
        if (!visited.insert(current).second) continue;
        if (current.type_is< void_type >() || is_ref(current) || is_ptr(current) || current.type_is< nvalue_slot >() || current.type_is< dvalue_slot >() ||
            current.type_is< readonly_constant >() || current.type_is< procedure_type >() || current.type_is< array_initializer_type >()) continue;
        if (std::optional< instanciation_reference > coroutine = coroutine_frame_specialization(current))
        {
            co_await co_add_captures(*coroutine);
            continue;
        }
        if (current.type_is< attached_type_reference >())
        {
            pending.push_back(current.get_as< attached_type_reference >().carrying_type);
            continue;
        }
        if (current.type_is< array_type >())
        {
            pending.push_back(current.get_as< array_type >().element_type);
            continue;
        }
        if (current.type_is< composite_type >())
        {
            for (std::pair< std::string const, type_symbol > const& field : current.get_as< composite_type >().fields)
                pending.push_back(field.second);
            continue;
        }
        if (current.type_is< storage >())
        {
            for (type_symbol const& alternative : current.get_as< storage >().storable_types) pending.push_back(alternative);
            continue;
        }
        class_kind kind = co_await rpnx::querygraph::request< class_type_query >(current);
        if (kind == class_kind::struct_ || kind == class_kind::generic || kind == class_kind::generic_ref)
        {
            std::vector< struct_field_declaration > const& fields = co_await rpnx::querygraph::request< struct_field_declaration_list_query >(current);
            for (struct_field_declaration const& field : fields)
            {
                std::optional< type_symbol > const& type = co_await rpnx::querygraph::request< lookup_query >(
                    contextual_type_reference{.context = current, .type = field.type});
                if (!type.has_value())
                    throw semantic_compilation_error("Stored field type could not be resolved in " + to_string(current) + ": " + field.name);
                pending.push_back(*type);
            }
            std::vector< struct_base_declaration > const& bases = co_await rpnx::querygraph::request< struct_direct_bases_query >(current);
            for (struct_base_declaration const& base : bases) pending.push_back(base.base_type);
        }
        else if (kind == class_kind::union_)
        {
            union_info const& info = co_await rpnx::querygraph::request< union_info_query >(current);
            if (info.properties.is_inline)
                for (union_option_info const& alternative : info.options) pending.push_back(alternative.type);
        }
        else if (kind == class_kind::variant)
        {
            variant_info const& info = co_await rpnx::querygraph::request< variant_info_query >(current);
            if (info.properties.is_inline)
                for (type_symbol const& alternative : info.alternatives) pending.push_back(alternative);
        }
    }
}

rpnx::querygraph::coroutine< quxlang::coroutine_frame_spec > quxlang::coroutine_frame_impl(instanciation_reference input)
{
    coroutine_storage_dependencies storage_dependencies{.frame = coroutine_frame_symbol(input)};
    co_await storage_dependencies.co_add_captures(input);
    co_await storage_dependencies.co_validate();
    vmir2::functanoid_routine3 const& routine = co_await rpnx::querygraph::request< vm_procedure3_query >(input);
    if (!routine.coroutine.has_value())
    {
        throw semantic_compilation_error("FRAME_TYPE requires a concrete COROUTINE specialization");
    }
    machine_target_info const& machine = co_await rpnx::querygraph::request< machine_info_query >(std::monostate{});
    if (cpu_is_layoutless(machine.cpu_type))
    {
        throw semantic_compilation_error("Coroutine frame placement requires a target with native object layout");
    }
    vmir2::coroutine_vmir_analysis logical = vmir2::coroutine_analysis_input(routine);
    logical.input.maximum_frame_size = machine.maximum_object_extent();
    type_symbol continuation_type = int_type{.bits = 32, .has_sign = false};
    class_placement_info const& continuation_placement = co_await rpnx::querygraph::request< class_placement_info_query >(continuation_type);
    logical.input.header_size = continuation_placement.size;
    logical.input.header_alignment = continuation_placement.alignment;

    // The first pass selects persistent slots before requesting their potentially recursive layouts.
    logical.input.refinement_work_limit = 0;
    vmir2::coroutine_frame_analysis selected = vmir2::analyze_coroutine_frame(logical.input);
    std::uint64_t refinement_limit = vmir2::coroutine_refinement_work_limit(selected.statistics.input_size);
    std::uint64_t remaining_work = refinement_limit;
    bool summary_exhausted = false;
    auto spend_summary = [&](std::uint64_t amount)
    {
        if (amount > remaining_work)
        {
            summary_exhausted = true;
            return false;
        }
        remaining_work -= amount;
        return true;
    };
    std::map< type_symbol, vmir2::invocation_borrow_summary > call_summaries;
    std::set< type_symbol > inspected_calls;
    std::uint64_t summary_entries = 0;
    // One callee's scratch and all retained parameter summaries fit a linear allowance.
    std::uint64_t memory_limit = selected.statistics.input_size;
    for (vmir2::executable_block const& block : routine.blocks)
    {
        if (!spend_summary(1)) break;
        for (vmir2::vm_instruction const& instruction : block.instructions)
        {
            if (!spend_summary(1)) break;
            if (!instruction.type_is< vmir2::invoke >()) continue;
            vmir2::invoke const& invocation = instruction.as< vmir2::invoke >();
            if (!inspected_calls.insert(invocation.what).second) continue;
            instanciation_reference const& callable = invocation.what.as< instanciation_reference >();
            if (coroutine_frame_operation_symbol(callable.temploid.templexoid).has_value()) continue;
            builtin_function_kind builtin = co_await rpnx::querygraph::request< function_builtin_query >(callable.temploid);
            if (builtin == builtin_function_kind::not_builtin || builtin == builtin_function_kind::builtin_intrinsic) continue;
            vmir2::functanoid_routine3 const& callee = co_await rpnx::querygraph::request< vm_procedure3_query >(callable);
            if (callee.local_types.size() > memory_limit) continue;
            vmir2::local_borrow_analysis borrows = vmir2::analyze_local_borrows(callee, {}, &remaining_work, memory_limit);
            if (!borrows.complete)
            {
                summary_exhausted = true;
                break;
            }
            vmir2::invocation_borrow_summary summary;
            bool complete = true;
            for (std::size_t position = 0; position < callee.parameters.positional.size(); ++position)
            {
                if (!spend_summary(1) || summary_entries == memory_limit)
                {
                    complete = false;
                    break;
                }
                ++summary_entries;
                if (!borrows.escaped_values.at(callee.parameters.positional[position].local_index)) summary.nonescaping_positional.insert(position);
            }
            for (std::pair< std::string const, vmir2::routine_parameter > const& parameter : callee.parameters.named)
            {
                if (!complete || !spend_summary(1) || summary_entries == memory_limit)
                {
                    complete = false;
                    break;
                }
                ++summary_entries;
                if (!borrows.escaped_values.at(parameter.second.local_index)) summary.nonescaping_named.insert(parameter.first);
            }
            if (complete) call_summaries.emplace(invocation.what, std::move(summary));
        }
        if (summary_exhausted) break;
    }
    if (!call_summaries.empty())
    {
        vmir2::local_borrow_analysis refined = vmir2::analyze_local_borrows(routine, call_summaries, &remaining_work, memory_limit);
        if (refined.complete && spend_summary(selected.statistics.input_size))
        {
            for (std::size_t index = 0; index < logical.input.objects.size(); ++index)
            {
                vmir2::local_index local(index);
                if (local == routine.coroutine->frame || local == routine.coroutine->controller || logical.borrowed_locals.contains(local)) continue;
                logical.input.objects[index].lifetime_complete = !refined.escaped_storage[index];
            }
            selected = vmir2::analyze_coroutine_frame(logical.input);
        }
        else summary_exhausted = true;
    }
    std::uint64_t summary_work = refinement_limit - remaining_work;
    logical.alias_work += summary_work;
    logical.input.refinement_work_limit = remaining_work;
    std::map< vmir2::local_index, type_symbol > payload_types;
    for (std::size_t index = 0; index < selected.objects.size(); ++index)
    {
        if (!selected.objects[index].offset.has_value()) continue;
        vmir2::local_index local(index);
        type_symbol payload = routine.local_types[index].type;
        if (logical.borrowed_parameters.contains(local))
        {
            type_symbol const& formal_type = logical.borrowed_parameters.at(local);
            type_symbol target = formal_type.type_is< nvalue_slot >() ? formal_type.as< nvalue_slot >().target : formal_type.as< dvalue_slot >().target;
            payload = ptrref_type{.target = std::move(target), .ptr_class = pointer_class::ref, .qual = qualifier::mut};
        }
        else if (logical.saved_array_element_addresses.contains(local))
        {
            payload = make_mref(payload);
        }
        else if (payload.type_is< nvalue_slot >() || payload.type_is< dvalue_slot >())
        {
            throw semantic_compilation_error("A coroutine output binding requires an owning persistent storage destination");
        }
        storage_dependencies.pending.push_back(payload);
        co_await storage_dependencies.co_validate();
        class_placement_info const& placement = co_await rpnx::querygraph::request< class_placement_info_query >(payload);
        logical.input.objects[index].size = placement.size;
        logical.input.objects[index].alignment = std::max(placement.alignment, routine.local_types[index].minimum_alignment);
        if (!logical.input.objects[index].requires_stable_address)
        {
            logical.input.objects[index].relocatable = co_await rpnx::querygraph::request< type_is_trivially_relocatable_query >(payload);
        }
        payload_types.emplace(local, std::move(payload));
    }
    vmir2::coroutine_frame_analysis physical;
    try
    {
        physical = vmir2::analyze_coroutine_frame(logical.input);
    }
    catch (std::overflow_error const& error)
    {
        throw semantic_compilation_error(error.what());
    }
    coroutine_frame_info output;
    output.fields.push_back({.name = "__CONTINUATION", .type = continuation_type, .minimum_alignment = continuation_placement.alignment});
    output.layout_fields.push_back({.name = "__CONTINUATION", .type = continuation_type, .declaration_ordinal = 0, .offset = 0});
    output.borrowed_locals = std::move(logical.borrowed_locals);
    output.borrowed_parameters = std::move(logical.borrowed_parameters);
    for (std::size_t index = 0; index < physical.objects.size(); ++index)
    {
        vmir2::coroutine_object_placement const& placement = physical.objects[index];
        if (!placement.offset.has_value()) continue;
        vmir2::local_index local(index);
        type_symbol storage_type = storage{.storable_types = {payload_types.at(local)}};
        std::string name = coroutine_storage_field_name(index);
        std::size_t ordinal = output.fields.size();
        output.fields.push_back({.name = name, .type = storage_type, .minimum_alignment = logical.input.objects[index].alignment});
        output.layout_fields.push_back({.name = std::move(name), .type = std::move(storage_type), .declaration_ordinal = ordinal,
            .offset = static_cast< std::size_t >(*placement.offset)});
        output.local_fields.emplace(local, ordinal);
        output.local_offsets.emplace(local, *placement.offset);
        if (logical.saved_array_element_addresses.contains(local)) output.saved_array_element_addresses.insert(local);
    }
    output.size = physical.size;
    output.alignment = physical.alignment;
    output.baseline_size = physical.statistics.baseline_size;
    output.analysis_input_size = physical.statistics.input_size;
    output.analysis_snapshot_nodes = logical.snapshot_nodes;
    output.analysis_snapshot_operands = logical.snapshot_operands;
    output.analysis_alias_work = logical.alias_work;
    output.refinement_limit = refinement_limit;
    output.refinement_work = summary_work + physical.statistics.refinement_work;
    output.refinement_exhausted = summary_exhausted || physical.statistics.refinement_exhausted;
    output.refined_layout_selected = physical.statistics.refined_layout_selected;
    co_return output;
}
