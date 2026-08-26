// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/struct_runtime_info_spec.hpp>

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/manipulators/typeutils.hpp>

#include <algorithm>
#include <map>
#include <set>

namespace
{
    /** Returns the concrete storage type of a field, excluding free attached bindings. */
    auto attached_field_storage_type(quxlang::type_symbol const& field_type) -> std::optional< quxlang::type_symbol >
    {
        if (!quxlang::typeis< quxlang::attached_type_reference >(field_type))
        {
            return field_type;
        }
        quxlang::attached_type_reference const& attached = quxlang::as< quxlang::attached_type_reference >(field_type);
        if (quxlang::typeis< quxlang::void_type >(attached.carrying_type))
        {
            return std::nullopt;
        }
        return attached.carrying_type;
    }

    /** Tests whether a canonical subobject identity is present in an ordered identity list. */
    auto contains_subobject(std::vector< quxlang::struct_subobject_id > const& subobjects, quxlang::struct_subobject_id const& selected) -> bool
    {
        return std::find(subobjects.begin(), subobjects.end(), selected) != subobjects.end();
    }

    /** Returns the struct that owns a normalized virtual final-overrider symbol. */
    auto virtual_routine_owner(quxlang::type_symbol const& routine) -> quxlang::type_symbol
    {
        quxlang::type_symbol declaration = routine;
        if (quxlang::typeis< quxlang::temploid_reference >(declaration))
        {
            declaration = quxlang::as< quxlang::temploid_reference >(declaration).templexoid;
        }
        if (!quxlang::typeis< quxlang::submember >(declaration))
        {
            throw quxlang::compiler_bug("Virtual final overrider is not a struct member: " + quxlang::to_string(routine));
        }
        return quxlang::as< quxlang::submember >(declaration).of;
    }

    /** Tests whether one inheritance path begins with every step in another path. */
    auto inheritance_path_has_prefix(quxlang::struct_subobject_path const& path, quxlang::struct_subobject_path const& prefix) -> bool
    {
        return prefix.steps.size() <= path.steps.size() && std::equal(prefix.steps.begin(), prefix.steps.end(), path.steps.begin());
    }

    /** Replaces THISTYPE in a normalized virtual THIS parameter with its final-overrider owner. */
    auto concrete_virtual_this_type(quxlang::type_symbol this_type, quxlang::type_symbol const& owner) -> quxlang::type_symbol
    {
        if (quxlang::typeis< quxlang::thistype >(this_type))
        {
            return owner;
        }
        if (quxlang::typeis< quxlang::dvalue_slot >(this_type))
        {
            quxlang::dvalue_slot slot = quxlang::as< quxlang::dvalue_slot >(this_type);
            slot.target = concrete_virtual_this_type(std::move(slot.target), owner);
            return slot;
        }
        if (quxlang::typeis< quxlang::nvalue_slot >(this_type))
        {
            quxlang::nvalue_slot slot = quxlang::as< quxlang::nvalue_slot >(this_type);
            slot.target = concrete_virtual_this_type(std::move(slot.target), owner);
            return slot;
        }
        if (quxlang::typeis< quxlang::ptrref_type >(this_type))
        {
            quxlang::ptrref_type pointer = quxlang::as< quxlang::ptrref_type >(this_type);
            pointer.target = concrete_virtual_this_type(std::move(pointer.target), owner);
            return pointer;
        }
        return this_type;
    }

    /** Embeds a child-type-relative subobject identity in the surrounding complete object. */
    auto embed_phase_subobject(quxlang::struct_subobject_id const& child_subobject, quxlang::struct_subobject_id const& relative_subobject) -> quxlang::struct_subobject_id
    {
        if (relative_subobject.virtual_root.has_value())
        {
            return relative_subobject;
        }
        quxlang::struct_subobject_id result = child_subobject;
        result.nonvirtual_path.insert(result.nonvirtual_path.end(), relative_subobject.nonvirtual_path.begin(), relative_subobject.nonvirtual_path.end());
        return result;
    }
}

rpnx::querygraph::coroutine< quxlang::struct_runtime_info_spec > quxlang::struct_runtime_info_impl(type_symbol input)
{
    struct_inheritance_info const inheritance = co_await rpnx::querygraph::request< struct_inheritance_info_query >(input);
    struct_layout const complete_layout = co_await rpnx::querygraph::request< struct_layout_query >(input);
    struct_runtime_requirements const requirements = co_await rpnx::querygraph::request< struct_runtime_requirements_query >(input);
    struct_virtual_slots const virtual_slots = co_await rpnx::querygraph::request< struct_virtual_slots_query >(input);

    struct_runtime_info output{
        .complete_type = input,
        .requirements = requirements,
        .allocation_size = complete_layout.complete_size,
        .allocation_align = complete_layout.complete_align,
    };
    if (requirements.polymorphism == struct_polymorphism_kind::none)
    {
        co_return output;
    }

    std::map< struct_subobject_id, std::int64_t > offsets;
    for (struct_subobject_record const& subobject : inheritance.subobjects)
    {
        if (subobject.paths.empty())
        {
            throw compiler_bug("Inheritance subobject has no source path: " + to_string(subobject.type));
        }
        std::int64_t offset = 0;
        type_symbol current_type = input;
        for (struct_subobject_path_step const& step : subobject.paths.front().steps)
        {
            if (step.kind == inheritance_kind::virtual_)
            {
                std::vector< struct_virtual_base_layout_info >::const_iterator const virtual_layout = std::ranges::find_if(complete_layout.virtual_bases, [&](struct_virtual_base_layout_info const& candidate)
                {
                    return candidate.type == step.base_type;
                });
                if (virtual_layout == complete_layout.virtual_bases.end())
                {
                    throw compiler_bug("Canonical virtual base is absent from complete layout: " + to_string(step.base_type));
                }
                offset = virtual_layout->offset;
            }
            else
            {
                struct_layout const source_layout = co_await rpnx::querygraph::request< struct_layout_query >(current_type);
                std::vector< struct_base_layout_info >::const_iterator const direct_layout = std::ranges::find_if(source_layout.direct_bases, [&](struct_base_layout_info const& candidate)
                {
                    return candidate.declaration_ordinal == step.direct_base_ordinal;
                });
                if (direct_layout == source_layout.direct_bases.end())
                {
                    throw compiler_bug("Direct base ordinal is absent from source layout");
                }
                offset += direct_layout->offset;
            }
            current_type = step.base_type;
        }
        offsets.emplace(subobject.id, offset);
        output.subobjects.push_back(struct_runtime_subobject{
            .id = subobject.id,
            .type = subobject.type,
            .offset = offset,
            .has_runtime_header = contains_subobject(requirements.runtime_header_subobjects, subobject.id),
        });
        output.cast_records.push_back(struct_runtime_cast_record{
            .target_type = subobject.type,
            .target_subobject = subobject.id,
            .target_offset = offset,
        });
    }
    std::ranges::sort(output.cast_records, [](struct_runtime_cast_record const& lhs, struct_runtime_cast_record const& rhs)
    {
        if (lhs.target_type != rhs.target_type)
        {
            return lhs.target_type < rhs.target_type;
        }
        return lhs.target_subobject < rhs.target_subobject;
    });

    for (struct_runtime_subobject const& active : output.subobjects)
    {
        if (!active.has_runtime_header)
        {
            continue;
        }
        for (struct_phase_kind const phase_kind : {struct_phase_kind::steady, struct_phase_kind::construction, struct_phase_kind::destruction})
        {
            struct_phase_kind const child_phase_kind = phase_kind == struct_phase_kind::destruction ? struct_phase_kind::destruction : struct_phase_kind::construction;
            struct_phase_descriptor_group group;
            group.phase = struct_phase_key{
                .kind = phase_kind,
                .active_subobject = active.id,
                .active_type = active.type,
            };

            std::vector< struct_field > const fields = co_await rpnx::querygraph::request< struct_field_list_query >(active.type);
            struct_layout const active_layout = co_await rpnx::querygraph::request< struct_layout_query >(active.type);
            group.field_transitions.resize(fields.size());
            for (std::size_t field_ordinal = 0; field_ordinal < fields.size(); ++field_ordinal)
            {
                std::optional< type_symbol > const field_storage_type = attached_field_storage_type(fields.at(field_ordinal).type);
                if (!field_storage_type.has_value() || co_await rpnx::querygraph::request< symbol_type_query >(*field_storage_type) != symbol_kind::class_ || co_await rpnx::querygraph::request< class_type_query >(*field_storage_type) != class_kind::struct_)
                {
                    continue;
                }
                struct_runtime_requirements const field_runtime = co_await rpnx::querygraph::request< struct_runtime_requirements_query >(*field_storage_type);
                if (field_runtime.polymorphism == struct_polymorphism_kind::none)
                {
                    continue;
                }
                std::vector< struct_field_info >::const_iterator const field_layout = std::ranges::find_if(active_layout.fields, [&](struct_field_info const& candidate)
                {
                    return candidate.declaration_ordinal == field_ordinal;
                });
                if (field_layout == active_layout.fields.end())
                {
                    continue;
                }
                group.field_transitions.at(field_ordinal).header_assignments.push_back(struct_phase_header_assignment{
                    .header_offset = static_cast< std::int64_t >(field_layout->offset),
                    .descriptor = struct_phase_descriptor_key{
                        .complete_type = *field_storage_type,
                        .phase = struct_phase_key{.kind = child_phase_kind, .active_type = *field_storage_type},
                    },
                });
            }

            struct_inheritance_info const active_inheritance = co_await rpnx::querygraph::request< struct_inheritance_info_query >(active.type);
            group.direct_base_transitions.resize(active_inheritance.direct_bases.size());
            for (struct_base_declaration const& base : active_inheritance.direct_bases)
            {
                if (base.kind == inheritance_kind::virtual_)
                {
                    continue;
                }
                struct_runtime_requirements const base_runtime = co_await rpnx::querygraph::request< struct_runtime_requirements_query >(base.base_type);
                if (base_runtime.polymorphism == struct_polymorphism_kind::none)
                {
                    continue;
                }
                std::vector< struct_base_layout_info >::const_iterator const base_layout = std::ranges::find_if(active_layout.direct_bases, [&](struct_base_layout_info const& candidate)
                {
                    return candidate.declaration_ordinal == base.declaration_ordinal;
                });
                QUXLANG_COMPILER_BUG_IF(base_layout == active_layout.direct_bases.end(), "Runtime base transition has no layout entry");
                struct_subobject_id child_id = active.id;
                child_id.nonvirtual_path.push_back(base.declaration_ordinal);
                std::set< std::int64_t > assigned_header_offsets;
                for (struct_subobject_id const& relative_header : base_runtime.runtime_header_subobjects)
                {
                    struct_subobject_id const embedded_header = embed_phase_subobject(child_id, relative_header);
                    std::int64_t const header_offset = offsets.at(embedded_header) - active.offset;
                    if (!assigned_header_offsets.insert(header_offset).second)
                    {
                        continue;
                    }
                    group.direct_base_transitions.at(base.declaration_ordinal).header_assignments.push_back(struct_phase_header_assignment{
                        .header_offset = header_offset,
                        .descriptor = struct_phase_descriptor_key{
                            .complete_type = input,
                            .phase = struct_phase_key{.kind = child_phase_kind, .active_subobject = child_id, .active_type = base.base_type},
                            .source_subobject = embedded_header,
                        },
                    });
                }
            }

            group.virtual_base_transitions.resize(active_inheritance.virtual_base_order.size());
            for (std::size_t virtual_ordinal = 0; virtual_ordinal < active_inheritance.virtual_base_order.size(); ++virtual_ordinal)
            {
                type_symbol const& virtual_type = active_inheritance.virtual_base_order.at(virtual_ordinal);
                struct_runtime_requirements const base_runtime = co_await rpnx::querygraph::request< struct_runtime_requirements_query >(virtual_type);
                if (base_runtime.polymorphism == struct_polymorphism_kind::none)
                {
                    continue;
                }
                struct_subobject_id const child_id{.virtual_root = virtual_type};
                std::set< std::int64_t > assigned_header_offsets;
                for (struct_subobject_id const& relative_header : base_runtime.runtime_header_subobjects)
                {
                    struct_subobject_id const embedded_header = embed_phase_subobject(child_id, relative_header);
                    std::int64_t const header_offset = offsets.at(embedded_header) - active.offset;
                    if (!assigned_header_offsets.insert(header_offset).second)
                    {
                        continue;
                    }
                    group.virtual_base_transitions.at(virtual_ordinal).header_assignments.push_back(struct_phase_header_assignment{
                        .header_offset = header_offset,
                        .descriptor = struct_phase_descriptor_key{
                            .complete_type = input,
                            .phase = struct_phase_key{.kind = child_phase_kind, .active_subobject = child_id, .active_type = virtual_type},
                            .source_subobject = embedded_header,
                        },
                    });
                }
            }
            output.descriptor_groups.push_back(std::move(group));
        }
    }

    for (struct_virtual_slot const& slot : virtual_slots.slots)
    {
        for (struct_virtual_overrider const& overrider : slot.overriders)
        {
            type_symbol const target_owner = virtual_routine_owner(overrider.final_overrider);
            struct_subobject_record const* target_subobject = nullptr;
            std::size_t target_path_length = 0;
            for (struct_subobject_record const& candidate : inheritance.subobjects)
            {
                if (candidate.type != target_owner)
                {
                    continue;
                }
                for (struct_subobject_path const& candidate_path : candidate.paths)
                {
                    bool const reaches_source = std::ranges::any_of(overrider.source_paths, [&](struct_subobject_path const& source_path)
                    {
                        return inheritance_path_has_prefix(source_path, candidate_path);
                    });
                    if (reaches_source && (target_subobject == nullptr || candidate_path.steps.size() > target_path_length))
                    {
                        target_subobject = &candidate;
                        target_path_length = candidate_path.steps.size();
                    }
                }
            }
            if (target_subobject == nullptr)
            {
                throw compiler_bug("Virtual final overrider owner is not an enclosing receiver subobject: " + to_string(overrider.final_overrider));
            }
            initialization_reference target_initialization;
            if (slot.key.signature.name == "DESTRUCTOR")
            {
                struct_runtime_requirements const target_requirements = co_await rpnx::querygraph::request< struct_runtime_requirements_query >(target_owner);
                target_initialization.initializee = submember{.of = target_owner, .name = target_requirements.polymorphism == struct_polymorphism_kind::virtual_polymorphic ? "FULLOBJECT_DESTRUCTOR" : "DESTRUCTOR"};
            }
            else
            {
                target_initialization.initializee = overrider.final_overrider;
            }
            target_initialization.parameters.named["THIS"] = make_type_instantiation(concrete_virtual_this_type(slot.key.signature.this_parameter, target_owner));
            for (type_symbol const& positional : slot.key.signature.positional_parameters)
            {
                target_initialization.parameters.positional.push_back(make_type_instantiation(positional));
            }
            for (std::pair< std::string const, type_symbol > const& named : slot.key.signature.named_parameters)
            {
                target_initialization.parameters.named[named.first] = make_type_instantiation(named.second);
            }
            std::optional< instanciation_reference > const target_routine = co_await rpnx::querygraph::request< functum_initialize_query >(std::move(target_initialization));
            if (!target_routine.has_value())
            {
                throw compiler_bug("Virtual final overrider could not be instantiated: " + to_string(overrider.final_overrider));
            }
            std::vector< struct_subobject_record >::const_iterator const source_subobject = std::ranges::find_if(inheritance.subobjects, [&](struct_subobject_record const& candidate)
            {
                return candidate.id == overrider.source_subobject;
            });
            if (source_subobject == inheritance.subobjects.end())
            {
                throw compiler_bug("Virtual slot source subobject is absent from inheritance information");
            }
            struct_virtual_slots const source_slots = co_await rpnx::querygraph::request< struct_virtual_slots_query >(source_subobject->type);
            std::vector< struct_virtual_slot >::const_iterator const source_slot = std::ranges::find_if(source_slots.slots, [&](struct_virtual_slot const& candidate)
            {
                return candidate.key == slot.key;
            });
            if (source_slot == source_slots.slots.end())
            {
                throw compiler_bug("Virtual slot is absent from its source subobject interface");
            }
            output.adjustment_thunks.push_back(struct_adjustment_thunk{
                .slot = slot.key,
                .slot_ordinal = source_slot->slot_ordinal,
                .source_subobject = overrider.source_subobject,
                .target_subobject = target_subobject->id,
                .target_routine = *target_routine,
                .receiver_adjustment = offsets.at(target_subobject->id) - offsets.at(overrider.source_subobject),
            });
        }
    }

    co_return output;
}
