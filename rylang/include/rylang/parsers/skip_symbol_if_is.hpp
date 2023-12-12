//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef RYLANG_PARSE_SYMBOL_IF_IS_HPP
#define RYLANG_PARSE_SYMBOL_IF_IS_HPP

#include <rylang/parsers/iter_parse_symbol.hpp>

#include <string_view>

namespace rylang::parsers
{
    template < typename It >
    inline bool skip_symbol_if_is(It& begin, It end, std::string_view symbol)
    {
        auto pos = iter_parse_symbol(begin, end);
        if (pos == begin)
        {
            return false;
        }
        if (std::string(begin, pos) == symbol)
        {
            begin = pos;
            return true;
        }
        return false;
    }
} // namespace rylang

#endif // PARSE_SYMBOL_IF_S_HPP
