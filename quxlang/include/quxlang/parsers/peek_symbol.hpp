//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef QUXLANG_PARSERS_PEEK_SYMBOL_HEADER_GUARD
#define QUXLANG_PARSERS_PEEK_SYMBOL_HEADER_GUARD

#include <quxlang/parsers/parse_symbol.hpp>

namespace quxlang::parsers
{

    // A function that peeks at the next symbol without adjusting the position of the input
    template < typename It >
    inline std::string peek_symbol(It pos, It end)
    {
        return parse_symbol(pos, end);
    }
} // namespace quxlang::parsers

#endif // PEEK_SYMBOL_HPP
