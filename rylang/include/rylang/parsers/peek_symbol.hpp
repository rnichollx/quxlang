//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef PEEK_SYMBOL_HPP
#define PEEK_SYMBOL_HPP

#include <rylang/parsers/parse_symbol.hpp>

namespace rylang::parsers
{

    // A function that peeks at the next symbol without adjusting the position of the input
    template < typename It >
    inline std::string peek_symbol(It pos, It end)
    {
        return parse_symbol(pos, end);
    }
} // namespace rylang::parsers

#endif // PEEK_SYMBOL_HPP
