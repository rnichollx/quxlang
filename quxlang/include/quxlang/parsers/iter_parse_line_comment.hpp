//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef ITER_PARSE_LINE_COMMENT_HPP
#define ITER_PARSE_LINE_COMMENT_HPP

#include <quxlang/parsers/iter_parse_symbol.hpp>

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
