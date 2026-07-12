// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/keywords.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/specs/union_info_spec.hpp>
#include <quxlang/queries/specs/variant_info_spec.hpp>

#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <utility>

namespace quxlang
{
    /// Normalizes common fusion modifiers after the declaration-specific default has been found.
    auto normalize_fusion_properties(bool is_inline, std::set< std::string > const& tags, std::optional< std::uint64_t > default_index,
                                     std::string const& type_name) -> fusion_properties
    {
        fusion_properties result;
        result.is_inline = is_inline;
        result.default_index = default_index;

        for (std::string const& tag : tags)
        {
            if (tag == keywords::never_valueless)
            {
                result.never_valueless = true;
            }
            else if (tag == keywords::valueless_default)
            {
                result.valueless_default = true;
            }
            else if (tag == keywords::no_default_copy)
            {
                result.generate_copy = false;
            }
            else if (tag == keywords::no_default_move)
            {
                result.generate_move = false;
            }
            else if (tag == keywords::no_default_assign)
            {
                result.generate_assignment = false;
            }
            else if (tag == keywords::no_default_swap)
            {
                result.generate_swap = false;
            }
            else
            {
                throw semantic_compilation_error("Unknown fusion keyword '" + tag + "' in " + type_name);
            }
        }

        if (result.never_valueless && result.valueless_default)
        {
            throw semantic_compilation_error("NEVER_VALUELESS and VALUELESS_DEFAULT cannot be combined in " + type_name);
        }
        if (result.valueless_default && result.default_index.has_value())
        {
            throw semantic_compilation_error("VALUELESS_DEFAULT cannot be combined with a DEFAULT alternative in " + type_name);
        }

        return result;
    }
} // namespace quxlang

rpnx::querygraph::coroutine< quxlang::union_info_spec > quxlang::union_info_impl(type_symbol input)
{
    ast2_symboid const symboid = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_union_declaration >(symboid))
    {
        throw compiler_bug("union_info requested for non-UNION type: " + to_string(input));
    }

    ast2_union_declaration const& declaration = as< ast2_union_declaration >(symboid);
    if (declaration.options.empty())
    {
        throw semantic_compilation_error("UNION must declare at least one option: " + to_string(input));
    }

    union_info result;
    std::set< std::string > names;
    std::optional< std::uint64_t > default_index;
    for (ast2_union_option_declaration const& option : declaration.options)
    {
        if (!names.insert(option.name).second)
        {
            throw semantic_compilation_error("Duplicate UNION option name '" + option.name + "' in " + to_string(input));
        }

        if (option.is_default)
        {
            if (default_index.has_value())
            {
                throw semantic_compilation_error("UNION declares more than one DEFAULT option: " + to_string(input));
            }
            default_index = static_cast< std::uint64_t >(result.options.size());
        }

        std::optional< type_symbol > const canonical_type = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
            .context = input,
            .type = option.type,
        });
        if (!canonical_type.has_value())
        {
            throw semantic_compilation_error("UNION option '" + option.name + "' has an unresolved type in " + to_string(input));
        }
        result.options.push_back(union_option_info{.name = option.name, .type = *canonical_type});
    }

    result.properties = normalize_fusion_properties(declaration.is_inline, declaration.keyword_tags, default_index, to_string(input));
    co_return result;
}

rpnx::querygraph::coroutine< quxlang::variant_info_spec > quxlang::variant_info_impl(type_symbol input)
{
    ast2_symboid const symboid = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_variant_declaration >(symboid))
    {
        throw compiler_bug("variant_info requested for non-VARIANT type: " + to_string(input));
    }

    ast2_variant_declaration const& declaration = as< ast2_variant_declaration >(symboid);
    if (declaration.entries.empty())
    {
        throw semantic_compilation_error("VARIANT must declare at least one alternative: " + to_string(input));
    }

    variant_info result;
    std::set< type_symbol > types;
    std::optional< std::uint64_t > default_index;
    for (ast2_variant_entry const& entry : declaration.entries)
    {
        std::optional< type_symbol > const canonical_type = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
            .context = input,
            .type = entry.type,
        });
        if (!canonical_type.has_value())
        {
            throw semantic_compilation_error("VARIANT has an unresolved alternative type in " + to_string(input));
        }
        if (!types.insert(*canonical_type).second)
        {
            throw semantic_compilation_error("VARIANT repeats alternative type " + to_string(*canonical_type) + " in " + to_string(input));
        }

        if (entry.is_default)
        {
            if (default_index.has_value())
            {
                throw semantic_compilation_error("VARIANT declares more than one DEFAULT alternative: " + to_string(input));
            }
            default_index = static_cast< std::uint64_t >(result.alternatives.size());
        }
        result.alternatives.push_back(*canonical_type);
    }

    result.properties = normalize_fusion_properties(declaration.is_inline, declaration.keyword_tags, default_index, to_string(input));
    co_return result;
}
