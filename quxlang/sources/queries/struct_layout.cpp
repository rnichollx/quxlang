// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/struct_layout_spec.hpp>

#include "quxlang/data/struct_field_declaration.hpp"
#include "quxlang/manipulators/struct_math.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <utility>

rpnx::querygraph::coroutine< quxlang::struct_layout_spec > quxlang::struct_layout_impl(type_symbol input)
{
    class_kind const input_kind = co_await rpnx::querygraph::request< class_type_query >(input);
    if (input_kind != class_kind::struct_ && input_kind != class_kind::generic && input_kind != class_kind::generic_ref)
    {
        throw compiler_bug("struct_layout received a non-struct class: " + to_string(input));
    }

    struct_layout output;
    ast2_symboid const symboid = co_await rpnx::querygraph::request< symboid_query >(input);
    bool is_ipc = false;
    if (typeis< ast2_struct_declaration >(symboid))
    {
        ast2_struct_declaration const& declaration = as< ast2_struct_declaration >(symboid);
        is_ipc = declaration.is_ipc;
    }

    struct_inheritance_info inheritance = co_await rpnx::querygraph::request< struct_inheritance_info_query >(input);
    struct_runtime_requirements runtime_requirements = co_await rpnx::querygraph::request< struct_runtime_requirements_query >(input);
    machine_target_info machine = co_await rpnx::querygraph::request< machine_info_query >(std::monostate{});
    std::vector< struct_field > struct_fields = co_await rpnx::querygraph::request< struct_field_list_query >(input);
    struct pending_struct_field
    {
        struct_field field;
        class_placement_info placement;
        std::size_t declaration_ordinal;
    };

    // Keep each field adjacent to its source ordinal while selecting physical placement order.
    std::vector< pending_struct_field > pending_fields;
    pending_fields.reserve(struct_fields.size());
    for (std::size_t field_ordinal = 0; field_ordinal < struct_fields.size(); ++field_ordinal)
    {
        struct_field& field = struct_fields.at(field_ordinal);
        class_placement_info const placement = co_await rpnx::querygraph::request< class_placement_info_query >(field.type);
        pending_fields.push_back(pending_struct_field{
            .field = std::move(field),
            .placement = placement,
            .declaration_ordinal = field_ordinal,
        });
    }

    if (!is_ipc)
    {
        std::stable_sort(pending_fields.begin(), pending_fields.end(), [](pending_struct_field const& lhs, pending_struct_field const& rhs)
        {
            return lhs.placement.alignment > rhs.placement.alignment;
        });
    }

    std::uint64_t nonvirtual_cursor = 0;
    std::set< std::pair< type_symbol, std::uint64_t > > occupied_base_addresses;
    std::map< std::size_t, std::size_t > direct_layout_index_by_ordinal;
    for (struct_base_declaration const& base : inheritance.direct_bases)
    {
        direct_layout_index_by_ordinal.emplace(base.declaration_ordinal, output.direct_bases.size());
        struct_subobject_id subobject;
        if (base.kind == inheritance_kind::virtual_)
        {
            subobject.virtual_root = base.base_type;
        }
        else
        {
            subobject.nonvirtual_path.push_back(base.declaration_ordinal);
        }
        output.direct_bases.push_back(struct_base_layout_info{
            .subobject = std::move(subobject),
            .selector_name = base.selector_name,
            .type = base.base_type,
            .kind = base.kind,
            .declaration_ordinal = base.declaration_ordinal,
        });
    }

    std::optional< std::size_t > primary_base_ordinal;
    if (runtime_requirements.primary_base_candidate.has_value() && !runtime_requirements.primary_base_candidate->nonvirtual_path.empty())
    {
        primary_base_ordinal = runtime_requirements.primary_base_candidate->nonvirtual_path.front();
    }
    if (runtime_requirements.polymorphism != struct_polymorphism_kind::none)
    {
        output.runtime_header = struct_runtime_header_layout{
            .offset = 0,
            .polymorphism = runtime_requirements.polymorphism,
            .primary_base = runtime_requirements.primary_base_candidate,
        };
        if (!primary_base_ordinal.has_value())
        {
            advance_to_alignment(nonvirtual_cursor, machine.pointer_align());
            output.runtime_header->offset = nonvirtual_cursor;
            nonvirtual_cursor += machine.pointer_size_bytes();
            output.nonvirtual_align = std::max< std::uint64_t >(output.nonvirtual_align, machine.pointer_align());
        }
    }

    auto place_nonvirtual_base = [&](struct_base_declaration const& base, struct_layout const& base_layout)
    {
        std::uint64_t offset;
        if (base_layout.nonvirtual_size == 0)
        {
            offset = 0;
            while (occupied_base_addresses.contains(std::pair{base.base_type, offset}))
            {
                ++offset;
            }
        }
        else
        {
            advance_to_alignment(nonvirtual_cursor, base_layout.nonvirtual_align);
            offset = nonvirtual_cursor;
            nonvirtual_cursor += base_layout.nonvirtual_size;
        }
        occupied_base_addresses.emplace(base.base_type, offset);
        output.nonvirtual_align = std::max(output.nonvirtual_align, base_layout.nonvirtual_align);
        output.direct_bases.at(direct_layout_index_by_ordinal.at(base.declaration_ordinal)).offset = static_cast< std::int64_t >(offset);
    };

    if (primary_base_ordinal.has_value())
    {
        struct_base_declaration const& primary_base = inheritance.direct_bases.at(*primary_base_ordinal);
        struct_layout primary_layout = co_await rpnx::querygraph::request< struct_layout_query >(primary_base.base_type);
        place_nonvirtual_base(primary_base, primary_layout);
        if (output.direct_bases.at(direct_layout_index_by_ordinal.at(*primary_base_ordinal)).offset != 0)
        {
            throw compiler_bug("Primary polymorphic base was not placed at offset zero");
        }
    }
    for (struct_base_declaration const& base : inheritance.direct_bases)
    {
        if (base.kind == inheritance_kind::virtual_ || base.declaration_ordinal == primary_base_ordinal)
        {
            continue;
        }
        struct_layout base_layout = co_await rpnx::querygraph::request< struct_layout_query >(base.base_type);
        place_nonvirtual_base(base, base_layout);
    }

    for (pending_struct_field& pending_field : pending_fields)
    {
        struct_field_info this_field;
        this_field.name = std::move(pending_field.field.name);
        this_field.type = std::move(pending_field.field.type);
        this_field.declaration_ordinal = pending_field.declaration_ordinal;
        advance_to_alignment(nonvirtual_cursor, pending_field.placement.alignment);
        this_field.offset = nonvirtual_cursor;
        nonvirtual_cursor += pending_field.placement.size;
        output.nonvirtual_align = std::max< std::uint64_t >(output.nonvirtual_align, pending_field.placement.alignment);
        output.fields.push_back(std::move(this_field));
    }

    if (is_ipc)
    {
        advance_to_alignment(nonvirtual_cursor, output.nonvirtual_align);
    }

    output.nonvirtual_size = nonvirtual_cursor;
    std::uint64_t complete_cursor = output.nonvirtual_size;
    output.complete_align = output.nonvirtual_align;
    std::map< type_symbol, std::uint64_t > virtual_base_offsets;
    for (std::size_t virtual_ordinal = 0; virtual_ordinal < inheritance.virtual_base_order.size(); ++virtual_ordinal)
    {
        type_symbol const& virtual_type = inheritance.virtual_base_order.at(virtual_ordinal);
        struct_layout virtual_layout = co_await rpnx::querygraph::request< struct_layout_query >(virtual_type);
        std::uint64_t offset;
        if (virtual_layout.nonvirtual_size == 0)
        {
            offset = 0;
            while (occupied_base_addresses.contains(std::pair{virtual_type, offset}))
            {
                ++offset;
            }
        }
        else
        {
            advance_to_alignment(complete_cursor, virtual_layout.nonvirtual_align);
            offset = complete_cursor;
            complete_cursor += virtual_layout.nonvirtual_size;
        }
        occupied_base_addresses.emplace(virtual_type, offset);
        virtual_base_offsets.emplace(virtual_type, offset);
        output.complete_align = std::max(output.complete_align, virtual_layout.nonvirtual_align);
        output.virtual_bases.push_back(struct_virtual_base_layout_info{
            .subobject = struct_subobject_id{.virtual_root = virtual_type},
            .type = virtual_type,
            .virtual_ordinal = virtual_ordinal,
            .offset = static_cast< std::int64_t >(offset),
        });
    }
    for (struct_base_declaration const& base : inheritance.direct_bases)
    {
        if (base.kind == inheritance_kind::virtual_)
        {
            output.direct_bases.at(direct_layout_index_by_ordinal.at(base.declaration_ordinal)).offset = static_cast< std::int64_t >(virtual_base_offsets.at(base.base_type));
        }
    }
    output.complete_size = complete_cursor;
    advance_to_alignment(output.complete_size, output.complete_align);

    co_return output;
}
