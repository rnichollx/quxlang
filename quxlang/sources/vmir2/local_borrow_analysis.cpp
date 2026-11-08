// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/vmir2/local_borrow_analysis.hpp>

#include <quxlang/vmir2/local_operands.hpp>
#include <quxlang/manipulators/typeutils.hpp>

#include <type_traits>

auto quxlang::vmir2::analyze_local_borrows(functanoid_routine3 const& routine,
    std::map< type_symbol, invocation_borrow_summary > const& calls, std::uint64_t* remaining_work, std::uint64_t maximum_alias_edges) -> local_borrow_analysis
{
    local_borrow_analysis result;
    /** Stops an optional proof before it exceeds its deterministic work allowance. */
    struct budget_exhaustion {};
    auto spend = [&](std::uint64_t amount)
    {
        if (remaining_work != nullptr)
        {
            if (amount > *remaining_work) throw budget_exhaustion{};
            *remaining_work -= amount;
        }
        result.work += amount;
    };
    try
    {
        for (std::size_t array = 0; array < 8; ++array) spend(routine.local_types.size());
        result.escaped_storage.resize(routine.local_types.size());
        std::vector< std::vector< local_index > > origins(routine.local_types.size());
        std::vector< std::vector< local_index > > derived(routine.local_types.size());
        std::vector< bool > stored_component(routine.local_types.size());
        std::vector< bool > output_reachable(routine.local_types.size());
        std::vector< bool > addressed(routine.local_types.size());
        std::vector< bool > visited(routine.local_types.size());
        std::vector< local_index > pending;
        auto pointer_value = [&](local_index local)
        {
            type_symbol const& type = routine.local_types.at(local).type;
            return type.type_is< ptrref_type >() || type.type_is< address_type >();
        };
        auto unknown_use = [&](local_index local)
        {
            spend(1);
            ++result.operands;
            if (pointer_value(local) && !visited.at(local))
            {
                visited[local] = true;
                pending.push_back(local);
            }
        };
        auto alias = [&](local_index source, local_index destination)
        {
            spend(1);
            if (result.alias_edges == maximum_alias_edges) throw budget_exhaustion{};
            ++result.alias_edges;
            origins.at(destination).push_back(source);
            derived.at(source).push_back(destination);
        };
        std::vector< local_index > outputs;
        auto output_parameter = [&](routine_parameter const& parameter)
        {
            spend(1);
            if (!parameter.type.type_is< nvalue_slot >()) return;
            stored_component.at(parameter.local_index) = true;
            if (!output_reachable.at(parameter.local_index))
            {
                output_reachable[parameter.local_index] = true;
                outputs.push_back(parameter.local_index);
            }
        };
        for (routine_parameter const& parameter : routine.parameters.positional) output_parameter(parameter);
        for (std::pair< std::string const, routine_parameter > const& parameter : routine.parameters.named) output_parameter(parameter.second);
        for (executable_block const& block : routine.blocks)
        {
            spend(1);
            for (vm_instruction const& instruction : block.instructions)
            {
                spend(1);
                instruction.apply_visitor< void >([&](auto& value)
                {
                    using operation = std::remove_cvref_t< decltype(value) >;
                    if constexpr (std::is_same_v< operation, make_reference >)
                    {
                        addressed.at(value.value_index) = true;
                        alias(value.value_index, value.reference_index);
                    }
                    else if constexpr (std::is_same_v< operation, make_pointer_to >)
                    {
                        addressed.at(value.of_index) = true;
                        alias(value.of_index, value.pointer_index);
                    }
                    else if constexpr (std::is_same_v< operation, copy_reference >) alias(value.from_index, value.to_index);
                    else if constexpr (std::is_same_v< operation, cast_ptrref >) alias(value.source_index, value.target_index);
                    else if constexpr (std::is_same_v< operation, dereference_pointer >) alias(value.from_pointer, value.to_reference);
                    else if constexpr (std::is_same_v< operation, access_field > || std::is_same_v< operation, access_array > || std::is_same_v< operation, access_pointer >)
                    {
                        alias(value.base_index, value.store_index);
                    }
                    else if constexpr (std::is_same_v< operation, pointer_arith >) alias(value.from, value.result);
                    else if constexpr (std::is_same_v< operation, inheritance_cast > || std::is_same_v< operation, struct_dynamic_cast >)
                    {
                        alias(value.source, value.result);
                    }
                    else if constexpr (std::is_same_v< operation, storage_pun >) alias(value.from_storage, value.to_reference);
                    else if constexpr (std::is_same_v< operation, storage_init_start > || std::is_same_v< operation, storage_deinit_start >)
                    {
                        alias(value.on_storage, value.target_value);
                    }
                    else if constexpr (std::is_same_v< operation, struct_init_start >)
                    {
                        addressed.at(value.on_value) = true;
                        for (struct_init_delegate const& delegate : value.delegates)
                        {
                            stored_component.at(delegate.value) = true;
                            alias(value.on_value, delegate.value);
                        }
                    }
                    else if constexpr (std::is_same_v< operation, array_init_start >)
                    {
                        if (value.source.template type_is< local_index >())
                        {
                            local_index source = value.source.template as< local_index >();
                            addressed.at(source) = true;
                            alias(source, value.initializer);
                        }
                        else alias(value.source.template as< array_init_storage_source >().storage_pointer, value.initializer);
                    }
                    else if constexpr (std::is_same_v< operation, array_init_element > || std::is_same_v< operation, array_deinit_previous >)
                    {
                        stored_component.at(value.target) = true;
                        alias(value.initializer, value.target);
                    }
                    else if constexpr (std::is_same_v< operation, fusion_storage_ref >)
                    {
                        addressed.at(value.subject) = true;
                        alias(value.subject, value.result);
                    }
                    else if constexpr (std::is_same_v< operation, relocate_value >)
                    {
                        if (pointer_value(value.source)) alias(value.source, value.target);
                    }
                    else if constexpr (std::is_same_v< operation, load_from_ref >)
                    {
                        if (pointer_value(value.to_value)) alias(value.from_reference, value.to_value);
                    }
                    else if constexpr (std::is_same_v< operation, invoke >)
                    {
                        spend(1);
                        std::map< type_symbol, invocation_borrow_summary >::const_iterator summary = calls.find(value.what);
                        auto named_nonescaping = [&](std::string const& name)
                        {
                            spend(1);
                            return summary != calls.end() && summary->second.nonescaping_named.contains(name);
                        };
                        for (std::size_t position = 0; position < value.args.positional.size(); ++position)
                        {
                            spend(1);
                            if (summary == calls.end() || !summary->second.nonescaping_positional.contains(position)) unknown_use(value.args.positional[position]);
                        }
                        for (std::pair< std::string const, local_index > const& argument : value.args.named)
                        {
                            if (!named_nonescaping(argument.first)) unknown_use(argument.second);
                        }
                        instatype parameters = value.what.template as< instanciation_reference >().params;
                        expand_pack_parameters(parameters);
                        auto borrowed_address = [&](local_index local)
                        {
                            spend(1);
                            addressed.at(local) = true;
                            if (!visited.at(local))
                            {
                                visited[local] = true;
                                pending.push_back(local);
                            }
                        };
                        auto parameter_address = [&](local_index local, type_symbol const& parameter)
                        {
                            spend(1);
                            std::optional< type_symbol > runtime = parameter_runtime_type(parameter);
                            if (runtime.has_value() && (runtime->type_is< nvalue_slot >() || runtime->type_is< dvalue_slot >())) borrowed_address(local);
                        };
                        std::size_t positional = 0;
                        for (parameter_instantiation const& parameter : parameters.positional)
                        {
                            spend(1);
                            type_symbol type = parameter_instantiation_type(parameter);
                            if (!parameter_runtime_type(type).has_value()) continue;
                            if (summary == calls.end() || !summary->second.nonescaping_positional.contains(positional))
                            {
                                parameter_address(value.args.positional.at(positional), type);
                            }
                            ++positional;
                        }
                        for (std::pair< std::string const, parameter_instantiation > const& parameter : parameters.named)
                        {
                            spend(1);
                            type_symbol type = parameter_instantiation_type(parameter.second);
                            if (parameter_runtime_type(type).has_value() && !named_nonescaping(parameter.first)) parameter_address(value.args.named.at(parameter.first), type);
                        }
                        std::map< std::string, local_index >::const_iterator output = value.args.named.find("RETURN");
                        if (output != value.args.named.end() && !named_nonescaping("RETURN")) borrowed_address(output->second);
                    }
                    else if constexpr (std::is_same_v< operation, store_to_ref >) unknown_use(value.from_value);
                    else if constexpr (std::is_same_v< operation, compare_exchange >) unknown_use(value.desired_value);
                    else if constexpr (std::is_same_v< operation, end_lifetime > ||
                                       std::is_same_v< operation, pointer_cmp > || std::is_same_v< operation, pointer_eq > ||
                                       std::is_same_v< operation, pointer_ne > || std::is_same_v< operation, pointer_diff > ||
                                       std::is_same_v< operation, address_cmp > || std::is_same_v< operation, address_mod > ||
                                       std::is_same_v< operation, destroy > || std::is_same_v< operation, begin_destruction > ||
                                       std::is_same_v< operation, complete_initialization >)
                    {
                    }
                    else visit_local_operands(value, unknown_use);
                });
            }
            if (block.terminator.has_value()) visit_local_operands(*block.terminator, unknown_use);
        }
        // Publishing a pointer-valued output or subobject also publishes its pointee.
        for (std::size_t position = 0; position < outputs.size(); ++position)
        {
            spend(1);
            local_index output = outputs[position];
            if (stored_component.at(output)) unknown_use(output);
            for (local_index child : derived.at(output))
            {
                spend(1);
                if (!output_reachable.at(child))
                {
                    output_reachable[child] = true;
                    outputs.push_back(child);
                }
            }
        }
        for (std::size_t position = 0; position < pending.size(); ++position)
        {
            local_index local = pending[position];
            spend(1);
            ++result.traversal_visits;
            if (addressed.at(local)) result.escaped_storage[local] = true;
            for (local_index origin : origins.at(local))
            {
                spend(1);
                ++result.traversal_visits;
                if (!visited.at(origin))
                {
                    visited[origin] = true;
                    pending.push_back(origin);
                }
            }
        }
        result.escaped_values = std::move(visited);
    }
    catch (budget_exhaustion const&)
    {
        result.complete = false;
        result.escaped_storage.clear();
        result.escaped_values.clear();
    }
    return result;
}
