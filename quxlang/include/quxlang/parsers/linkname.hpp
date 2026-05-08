// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef QUXLANG_PARSERS_LINKNAME_HEADER_GUARD
#define QUXLANG_PARSERS_LINKNAME_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include "keyword.hpp"
#include "quxlang/parsers/context.hpp"
#include "parse_whitespace_and_comments.hpp"
#include "symbol.hpp"
#include "string_literal.hpp"

#include <string>


namespace quxlang::parsers
{
    inline std::optional< std::string > try_parse_linkname(parsing_context& ctx)
    {
        auto& begin = ctx.iter_pos;
        auto end = ctx.iter_end;
        if (begin == end)
        {
            return std::nullopt;
        }

        if (!skip_keyword_if_is(begin, end, "LINKNAME"))
        {
            return std::nullopt;
        }

        skip_whitespace_and_comments(begin, end);

        if (!skip_symbol_if_is(begin, end, "("))
        {
            throw syntax_compilation_error("Expected '(' after LINKNAME");
        }

        skip_whitespace_and_comments(begin, end);

        auto str = try_parse_string_literal(begin, end);
        if (!str)
        {
            throw syntax_compilation_error("Expected string literal after LINKNAME(");
        }

        skip_whitespace_and_comments(begin, end);

        if (!skip_symbol_if_is(begin, end, ")"))
        {
            throw syntax_compilation_error("Expected ')' after LINKNAME(STRING_LITERAL");
        }

        return str;
    }
}

#endif //LINKNAME_HPP
