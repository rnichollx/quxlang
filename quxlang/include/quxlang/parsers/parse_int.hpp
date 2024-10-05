//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef QUXLANG_PARSERS_PARSE_INT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_INT_HEADER_GUARD
#include <quxlang/parsers/ctype.hpp>
#include <string>

namespace quxlang::parsers
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
