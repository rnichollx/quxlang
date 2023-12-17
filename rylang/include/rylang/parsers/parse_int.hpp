//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef PARSE_INT_HPP
#define PARSE_INT_HPP
#include <rylang/parsers/ctype.hpp>
#include <string>

namespace rylang::parsers
{
    template <typename It>
    constexpr std::string parse_int(It& pos, It end)
    {
        std::string out;
        while (pos != end && is_digit(*pos))
        {
            out += *pos;
            ++pos;
        }
        return out;
    }
}

#endif //PARSE_INT_HPP
