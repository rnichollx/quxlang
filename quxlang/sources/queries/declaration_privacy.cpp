// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/specs/declaration_privacy_spec.hpp>

namespace quxlang
{
    /** Returns whether a symboid establishes a CLASS privacy boundary. */
    static auto is_class_privacy_boundary(ast2_symboid const& symboid) -> bool
    {
        return symboid.type_is< ast2_struct_declaration >() || symboid.type_is< ast2_union_declaration >() || symboid.type_is< ast2_variant_declaration >() || symboid.type_is< ast2_implementation_declaration >();
    }

    /** Resolves one parsed privacy annotation relative to its declaration scope. */
    static auto resolve_privacy_scope(type_symbol const& declaration_parent, privacy_scope const& source_privacy, std::optional< source_location > declaration_location) -> rpnx::querygraph::coroutine< declaration_privacy_spec >::cosubroutine< resolved_privacy_scope >
    {
        resolved_privacy_scope output{
            .declaration_location = std::move(declaration_location),
        };

        for (privacy_scope_entry const& entry : source_privacy.entries)
        {
            type_symbol resolved_context;
            if (entry.kind == privacy_scope_kind::module)
            {
                std::optional< type_symbol > const module = get_root_module(declaration_parent);
                if (!module.has_value())
                {
                    throw semantic_compilation_error("PRIVATE(MODULE) declaration has no enclosing module");
                }
                resolved_context = *module;
            }
            else if (entry.kind == privacy_scope_kind::class_)
            {
                std::optional< type_symbol > current = declaration_parent;
                while (current.has_value())
                {
                    ast2_symboid const symboid = co_await rpnx::querygraph::request< symboid_query >(*current);
                    if (is_class_privacy_boundary(symboid))
                    {
                        resolved_context = *current;
                        break;
                    }
                    current = type_parent(*current);
                }
                if (!current.has_value())
                {
                    throw semantic_compilation_error("PRIVATE(CLASS) declaration has no enclosing STRUCT, UNION, VARIANT, or IMPLEMENTATION" + source_location_suffix(entry.location));
                }
            }
            else
            {
                if (!entry.named_context.has_value())
                {
                    throw compiler_bug("Named privacy scope is missing its context");
                }
                std::optional< type_symbol > const canonical = co_await rpnx::querygraph::request< canonical_lookup_query >(contextual_type_reference{
                    .context = declaration_parent,
                    .type = *entry.named_context,
                });
                if (!canonical.has_value())
                {
                    throw semantic_compilation_error("Could not resolve named PRIVATE scope " + to_string(*entry.named_context) + source_location_suffix(entry.location));
                }
                resolved_context = *canonical;
            }

            if (!output.contexts.insert(std::move(resolved_context)).second)
            {
                throw semantic_compilation_error("Duplicate PRIVATE scope" + source_location_suffix(entry.location));
            }
        }

        co_return output;
    }

    /** Returns the declaration name and member/global spelling expected by a symbol. */
    static auto declaration_identity(type_symbol const& declaration) -> std::optional< std::pair< bool, std::string > >
    {
        if (declaration.type_is< subsymbol >())
        {
            return std::pair{false, declaration.get_as< subsymbol >().name};
        }
        if (declaration.type_is< submember >())
        {
            return std::pair{true, declaration.get_as< submember >().name};
        }
        return std::nullopt;
    }

    /** Returns whether a declaration wrapper has the requested identity. */
    static auto declaration_matches(subdeclaroid const& declaration, bool member, std::string const& name) -> bool
    {
        if (member && declaration.type_is< member_subdeclaroid >())
        {
            return declaration.get_as< member_subdeclaroid >().name == name;
        }
        if (!member && declaration.type_is< global_subdeclaroid >())
        {
            return declaration.get_as< global_subdeclaroid >().name == name;
        }
        return false;
    }

    /** Returns the declaration payload from a member or global wrapper. */
    static auto declaration_payload(subdeclaroid const& declaration) -> declaroid const&
    {
        if (declaration.type_is< member_subdeclaroid >())
        {
            return declaration.get_as< member_subdeclaroid >().decl;
        }
        return declaration.get_as< global_subdeclaroid >().decl;
    }

    /** Returns the source privacy from a member or global wrapper. */
    static auto source_privacy(subdeclaroid const& declaration) -> std::optional< privacy_scope > const&
    {
        if (declaration.type_is< member_subdeclaroid >())
        {
            return declaration.get_as< member_subdeclaroid >().privacy;
        }
        return declaration.get_as< global_subdeclaroid >().privacy;
    }

    /** Returns the source location from a member or global wrapper. */
    static auto declaration_location(subdeclaroid const& declaration) -> std::optional< source_location > const&
    {
        if (declaration.type_is< member_subdeclaroid >())
        {
            return declaration.get_as< member_subdeclaroid >().location;
        }
        return declaration.get_as< global_subdeclaroid >().location;
    }
} // namespace quxlang

rpnx::querygraph::coroutine< quxlang::declaration_privacy_spec > quxlang::declaration_privacy_impl(type_symbol input)
{
    type_symbol declaration = input;
    std::optional< std::uint64_t > selected_overload;
    bool has_selected_overload = false;
    if (declaration.type_is< instanciation_reference >())
    {
        declaration = declaration.get_as< instanciation_reference >().temploid;
    }
    if (declaration.type_is< temploid_reference >())
    {
        temploid_reference const selection = declaration.get_as< temploid_reference >();
        selected_overload = selection.overload_id.value_or(0);
        has_selected_overload = true;
        declaration = selection.templexoid;
    }

    std::optional< std::pair< bool, std::string > > const identity = declaration_identity(declaration);
    std::optional< type_symbol > const parent = type_parent(declaration);
    if (!identity.has_value() || !parent.has_value())
    {
        co_return std::nullopt;
    }

    ast2_symboid const parent_symboid = co_await rpnx::querygraph::request< symboid_query >(*parent);
    if (parent_symboid.type_is< ast2_interface_declaration >())
    {
        if (!identity->first || !has_selected_overload)
        {
            co_return std::nullopt;
        }

        std::vector< ast2_interface_function_declaration const* > matching_functions;
        for (ast2_interface_function_declaration const& function : parent_symboid.get_as< ast2_interface_declaration >().functions)
        {
            if (function.name == identity->second)
            {
                matching_functions.push_back(&function);
            }
        }

        std::size_t const selected_index = static_cast< std::size_t >(*selected_overload);
        if (selected_index >= matching_functions.size() || !matching_functions.at(selected_index)->privacy.has_value())
        {
            co_return std::nullopt;
        }
        ast2_interface_function_declaration const& selected_function = *matching_functions.at(selected_index);
        co_return co_await resolve_privacy_scope(*parent, *selected_function.privacy, selected_function.location);
    }

    std::vector< subdeclaroid > const& declarations = co_await rpnx::querygraph::request< active_subdeclaroids_query >(*parent);
    std::vector< subdeclaroid const* > matching;
    for (subdeclaroid const& candidate : declarations)
    {
        if (declaration_matches(candidate, identity->first, identity->second))
        {
            matching.push_back(&candidate);
        }
    }
    if (matching.empty())
    {
        co_return std::nullopt;
    }

    if (!has_selected_overload)
    {
        declaroid const& first_payload = declaration_payload(*matching.front());
        if (first_payload.type_is< ast2_function_declaration >() || first_payload.type_is< ast2_template_declaration >())
        {
            co_return std::nullopt;
        }
    }

    if (has_selected_overload)
    {
        std::size_t const index = static_cast< std::size_t >(*selected_overload);
        if (index >= matching.size())
        {
            co_return std::nullopt;
        }
        subdeclaroid const& selected = *matching.at(index);
        if (!source_privacy(selected).has_value())
        {
            co_return std::nullopt;
        }
        co_return co_await resolve_privacy_scope(*parent, *source_privacy(selected), declaration_location(selected));
    }

    std::optional< resolved_privacy_scope > first_privacy;
    bool first = true;
    for (subdeclaroid const* candidate : matching)
    {
        std::optional< resolved_privacy_scope > candidate_privacy;
        if (source_privacy(*candidate).has_value())
        {
            candidate_privacy = co_await resolve_privacy_scope(*parent, *source_privacy(*candidate), declaration_location(*candidate));
        }

        if (first)
        {
            first_privacy = std::move(candidate_privacy);
            first = false;
            continue;
        }
        if (first_privacy.has_value() != candidate_privacy.has_value() || (first_privacy.has_value() && first_privacy->contexts != candidate_privacy->contexts))
        {
            throw semantic_compilation_error("Reopened declaration " + to_string(declaration) + " has inconsistent PRIVATE scopes" + source_location_suffix(declaration_location(*candidate)));
        }
    }

    co_return first_privacy;
}
