//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef ITER_PARSE_NUMBER_HPP
#define ITER_PARSE_NUMBER_HPP

#include <rylang/parsers/ctype.hpp>

namespace rylang::parsers
{
    template < typename It >
    constexpr auto iter_parse_number(It begin, It end) -> It
    {
        auto pos = begin;
        if (pos != end && is_digit(*pos))
        {
            pos++;
        }
        else
        {
            return pos;
        }

        bool havedot = false;

        while (pos != end && is_digit(*pos) || (*pos == '.' && !havedot))
        {
            if (*pos == '.')
            {
                havedot = true;
            }
            pos++;
        }

        return pos;
    }
} // namespace rylang::parsers

#endif // ITER_PARSE_NUMBER_HPP
