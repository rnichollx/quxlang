// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_ITER_PARSE_LINE_COMMENT_HEADER_GUARD
#define QUXLANG_PARSERS_ITER_PARSE_LINE_COMMENT_HEADER_GUARD

#include <quxlang/parsers/symbol.hpp>

namespace quxlang::parsers
{
    template < typename It >
    auto iter_parse_line_comment(It begin, It end) -> It
    {
        auto sym_end = iter_parse_symbol(begin, end);
        std::string symbol = std::string(begin, sym_end);
        if (symbol == "//")
        {
            while (begin != end && *begin != '\n' && *begin != '\r')
            {
                ++begin;
            }
        }
        return begin;
    }
} // namespace quxlang

#endif // ITER_PARSE_LINE_COMMENT_HPP
