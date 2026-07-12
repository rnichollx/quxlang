// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_FUSION_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_FUSION_HEADER_GUARD

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/keywords.hpp>
#include <quxlang/parsers/context.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/symbol.hpp>
#include <quxlang/parsers/try_parse_name.hpp>

#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace quxlang::parsers
{
    std::vector< subdeclaroid > parse_subdeclaroids(parsing_context& ctx);
    std::optional< subdeclaroid > try_parse_subdeclaroid(parsing_context& ctx);

    /// Parses the modifiers shared by all fusion declarations.
    inline auto parse_fusion_keywords(parsing_context& ctx) -> std::set< std::string >
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        std::set< std::string > result;

        while (true)
        {
            skip_whitespace_and_comments(pos, end);
            std::string const keyword = next_keyword(pos, end);
            if (keyword.empty())
            {
                break;
            }
            if (!keywords::fusion_keywords.contains(keyword))
            {
                throw syntax_compilation_error("Unknown fusion keyword: " + keyword);
            }
            parse_keyword(pos, end);
            result.insert(keyword);
        }

        return result;
    }

    /// Parses one leading named UNION option, leaving non-option declarations untouched.
    inline auto try_parse_union_option(parsing_context& ctx) -> std::optional< ast2_union_option_declaration >
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;
        auto probe = pos;
        std::optional< std::pair< bool, std::string > > name = try_parse_name(probe, end);
        if (!name.has_value() || !name->first)
        {
            return std::nullopt;
        }

        skip_whitespace_and_comments(probe, end);
        if (!skip_keyword_if_is(probe, end, "OPTION"))
        {
            return std::nullopt;
        }

        pos = probe;
        ast2_union_option_declaration result;
        result.name = std::move(name->second);
        skip_whitespace_and_comments(pos, end);
        result.is_default = skip_keyword_if_is(pos, end, "DEFAULT");
        skip_whitespace_and_comments(pos, end);
        result.type = parse_type_symbol(ctx);
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw syntax_compilation_error("Expected ';' after UNION option");
        }
        result.location = ctx.get_location_optional(begin, pos);
        return result;
    }

    /// Parses a UNION or INLINE_UNION declaration after its owning name.
    inline auto try_parse_union_declaration(parsing_context& ctx) -> std::optional< ast2_union_declaration >
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;
        std::optional< std::string_view > const keyword = skip_keyword_if_one_of(pos, end, {"INLINE_UNION", "UNION"});
        if (!keyword.has_value())
        {
            return std::nullopt;
        }

        ast2_union_declaration result;
        result.is_inline = *keyword == "INLINE_UNION";
        result.keyword_tags = parse_fusion_keywords(ctx);
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "{"))
        {
            throw syntax_compilation_error("Expected '{' after " + std::string(*keyword));
        }

        while (true)
        {
            skip_whitespace_and_comments(pos, end);
            std::optional< ast2_union_option_declaration > option = try_parse_union_option(ctx);
            if (option.has_value())
            {
                result.options.push_back(std::move(*option));
                continue;
            }

            std::optional< subdeclaroid > declaration = try_parse_subdeclaroid(ctx);
            if (!declaration.has_value())
            {
                break;
            }
            result.declarations.push_back(std::move(*declaration));
        }

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "}"))
        {
            throw syntax_compilation_error("Expected '}' after UNION body");
        }
        result.location = ctx.get_location_optional(begin, pos);
        return result;
    }

    /// Parses a VARIANT or INLINE_VARIANT declaration after its owning name.
    inline auto try_parse_variant_declaration(parsing_context& ctx) -> std::optional< ast2_variant_declaration >
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;
        std::optional< std::string_view > const keyword = skip_keyword_if_one_of(pos, end, {"INLINE_VARIANT", "VARIANT"});
        if (!keyword.has_value())
        {
            return std::nullopt;
        }

        ast2_variant_declaration result;
        result.is_inline = *keyword == "INLINE_VARIANT";
        result.keyword_tags = parse_fusion_keywords(ctx);
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "["))
        {
            throw syntax_compilation_error("Expected '[' after " + std::string(*keyword));
        }

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "]"))
        {
            while (true)
            {
                auto entry_begin = pos;
                ast2_variant_entry entry;
                entry.type = parse_type_symbol(ctx);
                skip_whitespace_and_comments(pos, end);
                entry.is_default = skip_keyword_if_is(pos, end, "DEFAULT");
                entry.location = ctx.get_location_optional(entry_begin, pos);
                result.entries.push_back(std::move(entry));

                skip_whitespace_and_comments(pos, end);
                if (skip_symbol_if_is(pos, end, "]"))
                {
                    break;
                }
                if (!skip_symbol_if_is(pos, end, ","))
                {
                    throw syntax_compilation_error("Expected ',' or ']' in VARIANT list");
                }
                skip_whitespace_and_comments(pos, end);
            }
        }

        skip_whitespace_and_comments(pos, end);
        if (skip_symbol_if_is(pos, end, "{"))
        {
            result.declarations = parse_subdeclaroids(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "}"))
            {
                throw syntax_compilation_error("Expected '}' after VARIANT body");
            }
        }
        else if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw syntax_compilation_error("Expected ';' or associated declaration body after VARIANT list");
        }

        result.location = ctx.get_location_optional(begin, pos);
        return result;
    }
} // namespace quxlang::parsers

#endif // QUXLANG_PARSERS_TRY_PARSE_FUSION_HEADER_GUARD
