// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_CLASS_BODY_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_CLASS_BODY_HEADER_GUARD

#include <optional>
#include <utility>
#include <quxlang/ast2/ast2_type_map.hpp>
#include <quxlang/keywords.hpp>
#include <quxlang/parsers/declaration.hpp>
#include <quxlang/parsers/parse_keyword.hpp>
#include <quxlang/parsers/try_parse_class_function_declaration.hpp>
#include <quxlang/parsers/try_parse_class_variable_declaration.hpp>

namespace quxlang::parsers
{
    std::vector< subdeclaroid > parse_subdeclaroids(parsing_context& ctx);

    inline ast2_class_declaration parse_class_body(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        skip_whitespace_and_comments(pos, end);

        ast2_class_declaration result;

        while (true)
        {
            auto next_kw = parse_keyword(pos, end);

            if (next_kw.empty())
            {
                break;
            }

            if (keywords::class_keywords.find(next_kw) != keywords::class_keywords.end())
            {
                result.class_keywords.insert(next_kw);
            }
            else
            {
                throw std::logic_error("Unknown keyword in class keywords: " + next_kw);
            }

            skip_whitespace_and_comments(pos, end);
        }

        if (!skip_symbol_if_is(pos, end, "{"))
        {
            throw std::logic_error("Expected '{'");
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
            throw std::logic_error("Expected '}'");
        }

        result.location = ctx.get_location_optional(begin, pos);
        return result;
    }
} // namespace quxlang::parsers

#endif // PARSE_CLASS_BODY_HEADER_GUARD
