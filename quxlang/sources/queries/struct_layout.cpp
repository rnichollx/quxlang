// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/struct_layout_spec.hpp>

#include "quxlang/data/struct_field_declaration.hpp"
#include "quxlang/manipulators/struct_math.hpp"

#include <algorithm>
#include <utility>

rpnx::querygraph::coroutine< quxlang::struct_layout_spec > quxlang::struct_layout_impl(type_symbol input)
{
    if (co_await rpnx::querygraph::request< class_type_query >(input) != class_kind::struct_)
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

    std::vector< struct_field > struct_fields = co_await rpnx::querygraph::request< struct_field_list_query >(input);
    // Keep each field adjacent to the placement used for ordering and offset calculation.
    std::vector< std::pair< struct_field, class_placement_info > > pending_fields;
    pending_fields.reserve(struct_fields.size());
    for (struct_field& field : struct_fields)
    {
        class_placement_info const placement = co_await rpnx::querygraph::request< class_placement_info_query >(field.type);
        pending_fields.emplace_back(std::move(field), placement);
    }

    if (!is_ipc)
    {
        std::stable_sort(pending_fields.begin(), pending_fields.end(), [](std::pair< struct_field, class_placement_info > const& lhs, std::pair< struct_field, class_placement_info > const& rhs)
        {
            return lhs.second.alignment > rhs.second.alignment;
        });
    }

    for (std::pair< struct_field, class_placement_info >& pending_field : pending_fields)
    {
        struct_field_info this_field;
        this_field.name = std::move(pending_field.first.name);
        this_field.type = std::move(pending_field.first.type);
        advance_to_alignment(output.size, pending_field.second.alignment);
        this_field.offset = output.size;
        output.size += pending_field.second.size;
        output.align = std::max< std::uint64_t >(output.align, pending_field.second.alignment);
        output.fields.push_back(std::move(this_field));
    }

    if (is_ipc)
    {
        advance_to_alignment(output.size, output.align);
    }

    co_return output;
}
