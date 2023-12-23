//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef PARSE_TYPE_SYMBOL_HPP
#define PARSE_TYPE_SYMBOL_HPP

#include <rylang/data/qualified_symbol_reference.hpp>

#include <rylang/parsers/try_parse_type_symbol.hpp>

#include <optional>

namespace rylang::parsers
{
    template < typename It >
    std::optional< type_symbol > try_parse_type_symbol(It& pos, It end);

    template < typename It >
    type_symbol parse_type_symbol(It& pos, It end)
    {
        auto result = try_parse_type_symbol(pos, end);
        if (!result)
        {
            throw std::runtime_error("Expected type symbol");
        }
        return result.value();
    }

    inline type_symbol parse_type_symbol(std::string str)
    {
        auto pos = str.begin();
        auto end = str.end();
        return parse_type_symbol(pos, end);
    }
} // namespace rylang::parsers

#endif // PARSE_TYPE_SYMBOL_HPP
