// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_STRUCT_BODY_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_STRUCT_BODY_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

#include <optional>
#include <utility>
#include <quxlang/ast2/ast2_type_map.hpp>
#include <quxlang/keywords.hpp>
#include <quxlang/parsers/declaration.hpp>
#include <quxlang/parsers/parse_keyword.hpp>
#include <quxlang/parsers/try_parse_struct_function_declaration.hpp>
#include <quxlang/parsers/try_parse_struct_variable_declaration.hpp>

namespace quxlang::parsers
{
    std::vector< subdeclaroid > parse_subdeclaroids(parsing_context& ctx);

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

        if (!skip_symbol_if_is(pos, end, "{"))
        {
            throw syntax_compilation_error("Expected '{'");
        }

        skip_whitespace_and_comments(pos, end);
        auto subdecls = parse_subdeclaroids(ctx);

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
