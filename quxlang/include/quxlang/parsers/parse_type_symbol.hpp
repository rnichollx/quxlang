//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef PARSE_TYPE_SYMBOL_HPP
#define PARSE_TYPE_SYMBOL_HPP

#include <quxlang/data/type_symbol.hpp>

#include <quxlang/parsers/try_parse_type_symbol.hpp>

#include <optional>

namespace quxlang::parsers
{
    template < typename It >
    std::optional< type_symbol > try_parse_type_symbol(It& pos, It end);

    template < typename It >
    type_symbol parse_type_symbol(It& pos, It end)
    {
        auto result = try_parse_type_symbol(pos, end);
        if (!result)
        {
            throw std::logic_error("Expected type symbol");
        }
        return result.value();
    }

    inline type_symbol parse_type_symbol(std::string str)
    {
        auto pos = str.begin();
        auto end = str.end();
        auto result = parse_type_symbol(pos, end);
        if (pos != end)
        {
            std::string rest(pos, end);
            std::string next_symbol = quxlang::parsers::parse_symbol(pos, end);
            throw std::logic_error("Input not fully parsed");
        }
        return result;
    }
} // namespace quxlang::parsers

#endif // PARSE_TYPE_SYMBOL_HPP
