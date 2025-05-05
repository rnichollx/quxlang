// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_EXPRESSION_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_EXPRESSION_HEADER_GUARD

#include <quxlang/parsers/try_parse_expression.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::optional< expression > try_parse_expression(It& pos, It end);




    template < typename It >
    expression parse_expression(It& pos, It end)
    {
        std::string remaining = std::string(pos, end);
        if (auto output = try_parse_expression(pos, end); output)
        {
            return output.value();
        }
        throw std::logic_error("Expected expression");
    }

    inline expression parse_expression(std::string str)
    {
        auto it = str.begin();
        auto expr = parse_expression(it, str.end());
        if (it != str.end())
        {
            std::string remaining = std::string(it, str.end());
            throw std::logic_error("expected fully parsed expression");
        }

        return expr;
    }
} // namespace quxlang::parsers

#endif // PARSE_EXPRESSION_HPP
