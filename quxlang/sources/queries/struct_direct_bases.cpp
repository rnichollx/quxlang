// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/struct_direct_bases_spec.hpp>

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/data/contextual_type_reference.hpp>
#include <quxlang/keywords.hpp>

#include <set>
#include <string>

rpnx::querygraph::coroutine< quxlang::struct_direct_bases_spec > quxlang::struct_direct_bases_impl(type_symbol input)
{
    ast2_symboid input_symboid = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!input_symboid.type_is< ast2_struct_declaration >())
    {
        co_return {};
    }

    ast2_struct_declaration const& input_declaration = input_symboid.get_as< ast2_struct_declaration >();
    std::vector< subdeclaroid > active_declarations = co_await rpnx::querygraph::request< active_symboid_subdeclaroids_query >(input);
    std::set< std::string > member_names;
    std::size_t active_base_count = 0;
    bool has_anonymous_base = false;

    for (subdeclaroid const& declaration : active_declarations)
    {
        if (!declaration.type_is< member_subdeclaroid >())
        {
            continue;
        }
        member_subdeclaroid const& member = declaration.get_as< member_subdeclaroid >();
        if (!member.decl.type_is< ast2_base_declaration >())
        {
            member_names.insert(member.name);
            continue;
        }

        ++active_base_count;
        has_anonymous_base = has_anonymous_base || member.name.empty();
    }

    if (active_base_count != 0 && input_declaration.is_ipc)
    {
        throw semantic_compilation_error("IPC_STRUCT cannot declare a base: " + to_string(input));
    }
    if (has_anonymous_base && active_base_count != 1)
    {
        throw semantic_compilation_error("An anonymous base requires exactly one active direct base in " + to_string(input));
    }

    std::vector< struct_base_declaration > output;
    output.reserve(active_base_count);
    std::set< std::string > selector_names;
    std::set< type_symbol > direct_virtual_types;

    for (subdeclaroid const& declaration : active_declarations)
    {
        if (!declaration.type_is< member_subdeclaroid >())
        {
            continue;
        }
        member_subdeclaroid const& member = declaration.get_as< member_subdeclaroid >();
        if (!member.decl.type_is< ast2_base_declaration >())
        {
            continue;
        }

        ast2_base_declaration const& parsed_base = member.decl.get_as< ast2_base_declaration >();
        std::optional< std::string > selector_name = member.name.empty() ? std::nullopt : std::optional< std::string >(member.name);
        std::string location = source_location_suffix(member.location);
        if (parsed_base.kind == inheritance_kind::virtual_ && !selector_name.has_value())
        {
            throw semantic_compilation_error("A virtual base must have a selector name in " + to_string(input) + location);
        }
        if (selector_name.has_value())
        {
            if (!selector_names.insert(*selector_name).second)
            {
                throw semantic_compilation_error("Duplicate direct base selector '" + *selector_name + "' in " + to_string(input) + location);
            }
            if (member_names.contains(*selector_name))
            {
                throw semantic_compilation_error("Direct base selector '" + *selector_name + "' collides with a member in " + to_string(input) + location);
            }
        }

        std::optional< type_symbol > canonical_base = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
            .context = input,
            .type = parsed_base.base_type,
        });
        if (!canonical_base.has_value())
        {
            throw semantic_compilation_error("Direct base type could not be resolved in " + to_string(input) + ": " + to_string(parsed_base.base_type) + location);
        }
        if (*canonical_base == input)
        {
            throw semantic_compilation_error("Inheritance cycle: " + to_string(input) + " -> " + to_string(input) + location);
        }

        class_kind base_kind = co_await rpnx::querygraph::request< class_type_query >(*canonical_base);
        ast2_symboid base_symboid = co_await rpnx::querygraph::request< symboid_query >(*canonical_base);
        if (base_kind != class_kind::struct_ || !base_symboid.type_is< ast2_struct_declaration >())
        {
            throw semantic_compilation_error("Direct base is not a concrete STRUCT instantiation: " + to_string(*canonical_base) + location);
        }

        ast2_struct_declaration const& base_declaration = base_symboid.get_as< ast2_struct_declaration >();
        if (base_declaration.is_ipc)
        {
            throw semantic_compilation_error("IPC_STRUCT cannot be used as a base: " + to_string(*canonical_base) + location);
        }

        std::set< std::string > base_tags = co_await rpnx::querygraph::request< struct_tags_query >(*canonical_base);
        if (base_tags.contains(keywords::final))
        {
            throw semantic_compilation_error("Cannot derive from FINAL struct " + to_string(*canonical_base) + location);
        }
        if (parsed_base.kind == inheritance_kind::virtual_ && !direct_virtual_types.insert(*canonical_base).second)
        {
            throw semantic_compilation_error("Duplicate direct virtual base " + to_string(*canonical_base) + " in " + to_string(input) + location);
        }

        output.push_back(struct_base_declaration{
            .selector_name = std::move(selector_name),
            .base_type = std::move(*canonical_base),
            .kind = parsed_base.kind,
            .declaration_ordinal = output.size(),
            .location = member.location,
        });
    }

    co_return output;
}
