//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef PARSE_EXPRESSION_HPP
#define PARSE_EXPRESSION_HPP

#include <quxlang/parsers/try_parse_expression.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::optional< expression > try_parse_expression(It& pos, It end);




    template < typename It >
    expression parse_expression(It& pos, It end)
    {
        if (auto output = try_parse_expression(pos, end); output)
        {
            return output.value();
        }
        throw std::runtime_error("Expected expression");
    }

    inline expression parse_expression(std::string str)
    {
        auto it = str.begin();
        return parse_expression(it, str.end());
    }
} // namespace quxlang::parsers

#endif // PARSE_EXPRESSION_HPP
