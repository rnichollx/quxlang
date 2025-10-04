//
// Created by Ryan Nicholl on 2025-04-20.
//

#ifndef QUXLANG_PARSERS_FWD_HEADER_GUARD
#define QUXLANG_PARSERS_FWD_HEADER_GUARD

#include <optional>

namespace quxlang::parsers
{
    template < typename It >
    std::optional< function_block > try_parse_function_block(It& pos, It end);
}

#endif // FWD_HPP
