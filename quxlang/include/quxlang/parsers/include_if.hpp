// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_INCLUDE_IF_HEADER_GUARD
#define QUXLANG_PARSERS_INCLUDE_IF_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

#include <optional>
#include <string_view>
#include <string>
#include <utility>
#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/skip_whitespace.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/symbol.hpp>


namespace quxlang::parsers
{
    /** Parses a parenthesized declaration condition following the specified keyword. */
    inline std::optional< expression > try_parse_include_if(parsing_context& ctx, std::string_view keyword = "INCLUDE_IF")
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        if (!skip_keyword_if_is(pos, end, keyword))
        {
            return std::nullopt;
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw syntax_compilation_error("expected ( after " + std::string(keyword));
        }

        skip_whitespace_and_comments(pos, end);

        expression out = parse_expression(ctx);

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw syntax_compilation_error("expected ) after " + std::string(keyword) + " condition");
        }

        skip_whitespace_and_comments(pos, end);

        return std::move(out);
    }

} // namespace quxlang::parsers

#endif // RPNX_QUXLANG_INCLUDE_IF_HEADER
