//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef PARSE_EXPRESSION_HPP
#define PARSE_EXPRESSION_HPP

#include <rylang/parsers/try_parse_expression.hpp>

namespace rylang::parsers
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
} // namespace rylang::parsers

#endif // PARSE_EXPRESSION_HPP
