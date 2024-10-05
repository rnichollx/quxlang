// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef QUXLANG_PARSERS_LINKNAME_HEADER_GUARD
#define QUXLANG_PARSERS_LINKNAME_HEADER_GUARD
#include "keyword.hpp"
#include "parse_whitespace_and_comments.hpp"
#include "symbol.hpp"
#include "string_literal.hpp"

#include <string>


namespace quxlang::parsers
{
    template <typename It>
    std::optional< std::string > try_parse_linkname(It& begin, It end)
    {
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
            throw std::logic_error("Expected '(' after LINKNAME");
        }

        skip_whitespace_and_comments(begin, end);

        auto str = try_parse_string_literal(begin, end);
        if (!str)
        {
            throw std::logic_error("Expected string literal after LINKNAME(");
        }

        skip_whitespace_and_comments(begin, end);

        if (!skip_symbol_if_is(begin, end, ")"))
        {
            throw std::logic_error("Expected ')' after LINKNAME(STRING_LITERAL");
        }

        return str;
    }
}

#endif //LINKNAME_HPP