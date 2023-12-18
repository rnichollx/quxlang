//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef GET_SKIP_IDENTIFIER_HPP
#define GET_SKIP_IDENTIFIER_HPP
#include <string>

#include <rylang/parsers/iter_parse_identifier.hpp>

namespace rylang::parsers
{
    template < typename It >
    inline std::string parse_identifier(It& begin, It end)
    {
        auto pos = iter_parse_identifier(begin, end);
        if (pos == begin)
        {
            return {};
        }
        auto result = std::string(begin, pos);
        begin = pos;
        return result;
    }
}

#endif //GET_SKIP_IDENTIFIER_HPP
