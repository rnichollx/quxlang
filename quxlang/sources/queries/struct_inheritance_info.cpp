// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/struct_inheritance_info_spec.hpp>

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/keywords.hpp>

#include <algorithm>
#include <deque>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <utility>

rpnx::querygraph::coroutine< quxlang::struct_inheritance_info_spec > quxlang::struct_inheritance_info_impl(type_symbol input)
{
    std::map< type_symbol, std::vector< struct_base_declaration > > graph;
    std::deque< type_symbol > pending_types;
    std::set< type_symbol > queued_types;
    pending_types.push_back(input);
    queued_types.insert(input);

    while (!pending_types.empty())
    {
        type_symbol current_type = std::move(pending_types.front());
        pending_types.pop_front();
        std::vector< struct_base_declaration > direct_bases = co_await rpnx::querygraph::request< struct_direct_bases_query >(current_type);
        for (struct_base_declaration const& base : direct_bases)
        {
            if (queued_types.insert(base.base_type).second)
            {
                pending_types.push_back(base.base_type);
            }
        }
        graph.emplace(std::move(current_type), std::move(direct_bases));
    }

    std::map< type_symbol, std::uint8_t > visit_state;
    std::vector< type_symbol > cycle_stack;
    std::function< void(type_symbol const&) > validate_acyclic;
    validate_acyclic = [&](type_symbol const& current_type)
    {
        std::uint8_t& state = visit_state[current_type];
        if (state == 2)
        {
            return;
        }
        if (state == 1)
        {
            std::vector< type_symbol >::const_iterator cycle_begin = std::find(cycle_stack.begin(), cycle_stack.end(), current_type);
            std::string message = "Inheritance cycle";
            for (std::vector< type_symbol >::const_iterator current = cycle_begin; current != cycle_stack.end(); ++current)
            {
                message += current == cycle_begin ? ": " : " -> ";
                message += to_string(*current);
            }
            message += " -> " + to_string(current_type);
            throw semantic_compilation_error(std::move(message));
        }

        state = 1;
        cycle_stack.push_back(current_type);
        std::vector< struct_base_declaration > const& direct_bases = graph.at(current_type);
        for (struct_base_declaration const& base : direct_bases)
        {
            validate_acyclic(base.base_type);
        }
        cycle_stack.pop_back();
        state = 2;
    };
    validate_acyclic(input);

    std::map< type_symbol, struct_polymorphism_kind > polymorphism;
    for (std::pair< type_symbol const, std::vector< struct_base_declaration > > const& entry : graph)
    {
        std::set< std::string > tags = co_await rpnx::querygraph::request< struct_tags_query >(entry.first);
        bool is_polymorphic = tags.contains(keywords::polymorphic);
        bool is_virtual_polymorphic = tags.contains(keywords::virtual_polymorphic);
        if (is_polymorphic && is_virtual_polymorphic)
        {
            throw semantic_compilation_error("STRUCT cannot be both POLYMORPHIC and VIRTUAL_POLYMORPHIC: " + to_string(entry.first));
        }
        if (is_virtual_polymorphic)
        {
            polymorphism.emplace(entry.first, struct_polymorphism_kind::virtual_polymorphic);
        }
        else if (is_polymorphic)
        {
            polymorphism.emplace(entry.first, struct_polymorphism_kind::polymorphic);
        }
        else
        {
            polymorphism.emplace(entry.first, struct_polymorphism_kind::none);
        }
    }

    bool has_virtual_inheritance = false;
    for (std::pair< type_symbol const, std::vector< struct_base_declaration > > const& entry : graph)
    {
        struct_polymorphism_kind source_kind = polymorphism.at(entry.first);
        for (struct_base_declaration const& base : entry.second)
        {
            struct_polymorphism_kind base_kind = polymorphism.at(base.base_type);
            if (base.kind == inheritance_kind::virtual_)
            {
                has_virtual_inheritance = true;
                if (source_kind != struct_polymorphism_kind::virtual_polymorphic)
                {
                    throw semantic_compilation_error("VIRTUAL_BASE requires VIRTUAL_POLYMORPHIC on " + to_string(entry.first) + source_location_suffix(base.location));
                }
            }
            if (base_kind == struct_polymorphism_kind::virtual_polymorphic && source_kind != struct_polymorphism_kind::virtual_polymorphic)
            {
                throw semantic_compilation_error("A struct derived from VIRTUAL_POLYMORPHIC " + to_string(base.base_type) + " must be VIRTUAL_POLYMORPHIC: " + to_string(entry.first) + source_location_suffix(base.location));
            }
            if (base_kind == struct_polymorphism_kind::polymorphic && source_kind == struct_polymorphism_kind::none)
            {
                throw semantic_compilation_error("A struct derived from POLYMORPHIC " + to_string(base.base_type) + " must declare a polymorphic category: " + to_string(entry.first) + source_location_suffix(base.location));
            }
        }
    }

    struct inheritance_expansion
    {
        type_symbol type;
        struct_subobject_id id;
        struct_subobject_path path;
    };

    struct_inheritance_info output;
    output.complete_type = input;
    output.direct_bases = graph.at(input);
    output.polymorphism = polymorphism.at(input);
    output.has_virtual_inheritance = has_virtual_inheritance;

    std::map< struct_subobject_id, std::size_t > record_indices;
    std::deque< inheritance_expansion > pending_subobjects;
    pending_subobjects.push_back(inheritance_expansion{
        .type = input,
        .id = {},
        .path = {},
    });

    while (!pending_subobjects.empty())
    {
        inheritance_expansion current = std::move(pending_subobjects.front());
        pending_subobjects.pop_front();
        std::map< struct_subobject_id, std::size_t >::iterator existing = record_indices.find(current.id);
        std::size_t record_index;
        if (existing == record_indices.end())
        {
            record_index = output.subobjects.size();
            record_indices.emplace(current.id, record_index);
            output.subobjects.push_back(struct_subobject_record{
                .id = current.id,
                .type = current.type,
                .paths = {},
            });
        }
        else
        {
            record_index = existing->second;
            if (output.subobjects.at(record_index).type != current.type)
            {
                throw compiler_bug("Canonical inheritance subobject identity selected two types");
            }
        }

        std::vector< struct_subobject_path >& paths = output.subobjects.at(record_index).paths;
        if (std::find(paths.begin(), paths.end(), current.path) == paths.end())
        {
            paths.push_back(current.path);
        }

        for (struct_base_declaration const& base : graph.at(current.type))
        {
            struct_subobject_id child_id = current.id;
            if (base.kind == inheritance_kind::virtual_)
            {
                child_id.virtual_root = base.base_type;
                child_id.nonvirtual_path.clear();
            }
            else
            {
                child_id.nonvirtual_path.push_back(base.declaration_ordinal);
            }

            struct_subobject_path child_path = current.path;
            child_path.steps.push_back(struct_subobject_path_step{
                .direct_base_ordinal = base.declaration_ordinal,
                .kind = base.kind,
                .base_type = base.base_type,
            });
            pending_subobjects.push_back(inheritance_expansion{
                .type = base.base_type,
                .id = std::move(child_id),
                .path = std::move(child_path),
            });
        }
    }

    std::set< type_symbol > expanded_virtual_order_types;
    std::set< type_symbol > ordered_virtual_bases;
    std::function< void(type_symbol const&) > append_virtual_base_order;
    append_virtual_base_order = [&](type_symbol const& current_type)
    {
        if (!expanded_virtual_order_types.insert(current_type).second)
        {
            return;
        }
        for (struct_base_declaration const& base : graph.at(current_type))
        {
            append_virtual_base_order(base.base_type);
            if (base.kind == inheritance_kind::virtual_ && ordered_virtual_bases.insert(base.base_type).second)
            {
                output.virtual_base_order.push_back(base.base_type);
            }
        }
    };
    append_virtual_base_order(input);

    co_return output;
}
