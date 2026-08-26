// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/struct_member_lookup_spec.hpp>

#include <algorithm>
#include <functional>
#include <iterator>
#include <map>
#include <set>
#include <utility>

rpnx::querygraph::coroutine< quxlang::struct_member_lookup_spec > quxlang::struct_member_lookup_impl(struct_member_lookup_input input)
{
    struct_inheritance_info inheritance = co_await rpnx::querygraph::request< struct_inheritance_info_query >(input.static_type);
    std::set< type_symbol > hierarchy_types;
    for (struct_subobject_record const& subobject : inheritance.subobjects)
    {
        hierarchy_types.insert(subobject.type);
    }

    std::map< type_symbol, std::vector< struct_base_declaration > > direct_bases;
    std::map< type_symbol, bool > has_direct_member;
    for (type_symbol const& hierarchy_type : hierarchy_types)
    {
        direct_bases.emplace(hierarchy_type, co_await rpnx::querygraph::request< struct_direct_bases_query >(hierarchy_type));
        std::vector< subdeclaroid > declarations = co_await rpnx::querygraph::request< active_symboid_subdeclaroids_query >(hierarchy_type);
        bool has_match = false;
        for (subdeclaroid const& declaration : declarations)
        {
            if (declaration.type_is< member_subdeclaroid >() && declaration.get_as< member_subdeclaroid >().name == input.member_name)
            {
                has_match = true;
                break;
            }
        }
        has_direct_member.emplace(hierarchy_type, has_match);
    }

    struct lookup_position
    {
        type_symbol type;
        struct_subobject_id id;
        struct_subobject_path path;
    };

    std::function< std::vector< struct_member_lookup_candidate >(lookup_position const&) > search;
    search = [&](lookup_position const& current) -> std::vector< struct_member_lookup_candidate >
    {
        std::vector< struct_member_lookup_candidate > local_candidates;
        std::vector< struct_base_declaration > const& bases = direct_bases.at(current.type);
        for (struct_base_declaration const& base : bases)
        {
            if (base.selector_name != std::optional< std::string >{input.member_name})
            {
                continue;
            }

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
            local_candidates.push_back(struct_member_lookup_candidate{
                .kind = struct_member_candidate_kind::base_projection,
                .selected_declaration = submember{current.type, input.member_name},
                .receiver_type = base.base_type,
                .receiver_subobject = std::move(child_id),
                .receiver_path = std::move(child_path),
            });
        }
        if (!local_candidates.empty())
        {
            return local_candidates;
        }

        if (has_direct_member.at(current.type))
        {
            return {struct_member_lookup_candidate{
                .kind = struct_member_candidate_kind::declaration,
                .selected_declaration = submember{current.type, input.member_name},
                .receiver_type = current.type,
                .receiver_subobject = current.id,
                .receiver_path = current.path,
            }};
        }

        std::vector< struct_member_lookup_candidate > inherited_candidates;
        for (struct_base_declaration const& base : bases)
        {
            lookup_position child{
                .type = base.base_type,
                .id = current.id,
                .path = current.path,
            };
            if (base.kind == inheritance_kind::virtual_)
            {
                child.id.virtual_root = base.base_type;
                child.id.nonvirtual_path.clear();
            }
            else
            {
                child.id.nonvirtual_path.push_back(base.declaration_ordinal);
            }
            child.path.steps.push_back(struct_subobject_path_step{
                .direct_base_ordinal = base.declaration_ordinal,
                .kind = base.kind,
                .base_type = base.base_type,
            });
            std::vector< struct_member_lookup_candidate > child_candidates = search(child);
            inherited_candidates.insert(inherited_candidates.end(), std::make_move_iterator(child_candidates.begin()), std::make_move_iterator(child_candidates.end()));
        }
        return inherited_candidates;
    };

    std::vector< struct_member_lookup_candidate > candidates = search(lookup_position{
        .type = input.static_type,
        .id = {},
        .path = {},
    });
    std::vector< struct_member_lookup_candidate > unique_candidates;
    for (struct_member_lookup_candidate& candidate : candidates)
    {
        bool duplicate = std::any_of(unique_candidates.begin(), unique_candidates.end(), [&](struct_member_lookup_candidate const& existing)
        {
            return existing.kind == candidate.kind && existing.selected_declaration == candidate.selected_declaration && existing.receiver_subobject == candidate.receiver_subobject;
        });
        if (!duplicate)
        {
            unique_candidates.push_back(std::move(candidate));
        }
    }

    bool ambiguous = unique_candidates.size() > 1;
    co_return struct_member_lookup_result{
        .candidates = std::move(unique_candidates),
        .ambiguous = ambiguous,
    };
}
