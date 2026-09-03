// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_STRUCT_BODY_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_STRUCT_BODY_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

#include <iterator>
#include <optional>
#include <utility>
#include <quxlang/ast2/ast2_type_map.hpp>
#include <quxlang/keywords.hpp>
#include <quxlang/parsers/declaration.hpp>
#include <quxlang/parsers/doc.hpp>
#include <quxlang/parsers/include_if.hpp>
#include <quxlang/parsers/parse_privacy_scope.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>
#include <quxlang/parsers/parse_keyword.hpp>
#include <quxlang/parsers/parse_identifier.hpp>
#include <quxlang/parsers/try_parse_struct_function_declaration.hpp>
#include <quxlang/parsers/try_parse_struct_variable_declaration.hpp>

namespace quxlang::parsers
{
    std::optional< subdeclaroid > try_parse_subdeclaroid(parsing_context& ctx, std::optional< privacy_scope > privacy);

    /** Parses one direct base declaration without consuming non-base member syntax. */
    inline auto try_parse_struct_base_declaration(parsing_context& ctx, std::optional< privacy_scope > privacy) -> std::optional< member_subdeclaroid >
    {
        parsing_context trial = ctx;
        parse_iterator const begin = trial.iter_pos;
        if (!skip_symbol_if_is(trial.iter_pos, trial.iter_end, "."))
        {
            return std::nullopt;
        }

        ast2_base_declaration declaration;
        std::string member_name;
        std::optional< expression > include_if;
        std::optional< std::string > doc;
        bool anonymous = false;
        if (skip_keyword_if_is(trial.iter_pos, trial.iter_end, "BASE"))
        {
            anonymous = true;
            declaration.kind = inheritance_kind::nonvirtual;
            skip_whitespace_and_comments(trial.iter_pos, trial.iter_end);
            include_if = try_parse_include_if(trial);
            doc = try_parse_single_doc(trial);
        }
        else
        {
            std::string selector = parse_identifier(trial.iter_pos, trial.iter_end);
            if (selector.empty())
            {
                return std::nullopt;
            }
            member_name = std::move(selector);
            skip_whitespace_and_comments(trial.iter_pos, trial.iter_end);
            include_if = try_parse_include_if(trial);
            doc = try_parse_single_doc(trial);

            if (skip_keyword_if_is(trial.iter_pos, trial.iter_end, "BASE"))
            {
                declaration.kind = inheritance_kind::nonvirtual;
            }
            else if (skip_keyword_if_is(trial.iter_pos, trial.iter_end, "VIRTUAL_BASE"))
            {
                declaration.kind = inheritance_kind::virtual_;
            }
            else
            {
                return std::nullopt;
            }
        }

        if (privacy.has_value())
        {
            throw syntax_compilation_error("Base declarations cannot carry declaration privacy");
        }
        if (anonymous && declaration.kind == inheritance_kind::virtual_)
        {
            throw syntax_compilation_error("An anonymous base cannot be virtual");
        }

        skip_whitespace_and_comments(trial.iter_pos, trial.iter_end);
        declaration.base_type = parse_type_symbol(trial);
        skip_whitespace_and_comments(trial.iter_pos, trial.iter_end);
        if (!skip_symbol_if_is(trial.iter_pos, trial.iter_end, ";"))
        {
            throw syntax_compilation_error("Expected ';' after base type");
        }

        std::optional< source_location > location = ctx.get_location_optional(begin, trial.iter_pos);
        declaration.location = location;
        ctx.iter_pos = trial.iter_pos;
        return member_subdeclaroid{
            .decl = std::move(declaration),
            .name = std::move(member_name),
            .include_if = std::move(include_if),
            .doc = std::move(doc),
            .privacy = std::move(privacy),
            .location = std::move(location),
        };
    }

    /** Parses a STRUCT body while preserving direct bases as ordered semantic declarations. */
    inline auto parse_struct_subdeclaroids(parsing_context& ctx, std::optional< privacy_scope > inherited_privacy = std::nullopt) -> std::vector< subdeclaroid >
    {
        std::vector< subdeclaroid > output;
        while (true)
        {
            skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
            std::optional< privacy_scope > declaration_privacy = try_parse_privacy_scope(ctx);
            if (declaration_privacy.has_value())
            {
                if (inherited_privacy.has_value())
                {
                    throw syntax_compilation_error("PRIVATE declarations cannot be nested directly inside a PRIVATE block");
                }

                skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
                if (skip_symbol_if_is(ctx.iter_pos, ctx.iter_end, "{"))
                {
                    std::vector< subdeclaroid > nested = parse_struct_subdeclaroids(ctx, declaration_privacy);
                    skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
                    if (!skip_symbol_if_is(ctx.iter_pos, ctx.iter_end, "}"))
                    {
                        throw syntax_compilation_error("expected } after PRIVATE declaration block");
                    }
                    output.insert(output.end(), std::make_move_iterator(nested.begin()), std::make_move_iterator(nested.end()));
                    continue;
                }
            }

            std::optional< privacy_scope > effective_privacy = declaration_privacy.has_value() ? declaration_privacy : inherited_privacy;
            std::optional< member_subdeclaroid > base = try_parse_struct_base_declaration(ctx, effective_privacy);
            if (base.has_value())
            {
                output.push_back(std::move(*base));
                continue;
            }

            std::optional< subdeclaroid > declaration = try_parse_subdeclaroid(ctx, std::move(effective_privacy));
            if (!declaration.has_value())
            {
                if (declaration_privacy.has_value())
                {
                    throw syntax_compilation_error("expected declaration after PRIVATE scope");
                }
                break;
            }
            output.push_back(std::move(*declaration));
        }
        return output;
    }

    /** Parses the keyword tags and members following a STRUCT keyword. */
    inline ast2_struct_declaration parse_struct_body(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        skip_whitespace_and_comments(pos, end);

        ast2_struct_declaration result;

        while (true)
        {
            auto next_kw = parse_keyword(pos, end);

            if (next_kw.empty())
            {
                break;
            }

            if (keywords::struct_keywords.find(next_kw) != keywords::struct_keywords.end())
            {
                result.struct_keywords.insert(next_kw);
            }
            else
            {
                throw syntax_compilation_error("Unknown keyword in struct keywords: " + next_kw);
            }

            skip_whitespace_and_comments(pos, end);
        }

        if (result.struct_keywords.contains(keywords::rooted) && result.struct_keywords.contains(keywords::move_only))
        {
            throw syntax_compilation_error("ROOTED and MOVE_ONLY modifiers cannot be combined");
        }

        if (!skip_symbol_if_is(pos, end, "{"))
        {
            throw syntax_compilation_error("Expected '{'");
        }

        skip_whitespace_and_comments(pos, end);
        std::vector< subdeclaroid > subdecls = parse_struct_subdeclaroids(ctx);

        for (auto& decl : subdecls)
        {
            result.declarations.push_back(std::move(decl));
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "}"))
        {
            throw syntax_compilation_error("Expected '}'");
        }

        result.location = ctx.get_location_optional(begin, pos);
        return result;
    }
} // namespace quxlang::parsers

#endif // QUXLANG_PARSERS_PARSE_STRUCT_BODY_HEADER_GUARD
