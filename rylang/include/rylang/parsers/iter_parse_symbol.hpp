//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef RYLANG_ITER_PARSE_SYMBOL_HPP
#define RYLANG_ITER_PARSE_SYMBOL_HPP

#include <rylang/parsers/ctype.hpp>

namespace rylang::parsers
{
    template < typename It >
    constexpr auto iter_parse_symbol(It begin, It end) -> It
    {
        auto pos = begin;
        bool started = false;
        while (pos != end && !is_space(*pos) && !is_alpha(*pos) && !is_digit(*pos) && (!started || ((*pos != ')') && (*pos != '{' && *pos != '}') && (*pos != ',' && *pos != ';'))) && *pos != '_')
        {
            char c = *pos;
            ++pos;

            if (c == '(' || c == ')')
            {
                break;
            }
            started = true;
        }

        return pos;
    }
} // namespace rylang

#endif // ITER_PARSE_SYMBOL_HPP
