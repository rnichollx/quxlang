//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef QUXLANG_ITER_PARSE_SYMBOL_HPP
#define QUXLANG_ITER_PARSE_SYMBOL_HPP

#include <quxlang/parsers/ctype.hpp>

namespace quxlang::parsers
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
} // namespace quxlang

#endif // ITER_PARSE_SYMBOL_HPP
