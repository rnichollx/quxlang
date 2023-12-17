//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef PARSE_KEYWORD_HPP
#define PARSE_KEYWORD_HPP
#include <rylang/parsers/iter_parse_keyword.hpp>
#include <string>

namespace rylang::parsers
{
    template < typename It >
    std::string parse_keyword(It& pos, It end)
    {
        auto it = iter_parse_keyword(pos, end);
        std::string out(pos, it);
        pos = it;
        return out;
    }
} // namespace rylang::parsers

#endif // PARSE_KEYWORD_HPP
