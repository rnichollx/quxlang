//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef PARSE_SUBENTITY_HPP
#define PARSE_SUBENTITY_HPP
#include <quxlang/parsers/iter_parse_identifier.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/skip_whitespace.hpp>
#include <quxlang/parsers/symbol.hpp>
#include <set>
#include <string>

namespace quxlang::parsers
{
    template < typename It >
    inline std::string parse_subentity(It& begin, It end)
    {
        static std::set< std::string > subentity_keywords = {"CONSTRUCTOR", "DESTRUCTOR", "OPERATOR"};

        auto pos = iter_parse_identifier(begin, end);
        if (pos != begin)
        {
            auto result = std::string(begin, pos);
            begin = pos;
            return result;
        }

        std::string result;

        pos = iter_parse_keyword(begin, end);
        auto kw = std::string(begin, pos);

        if (!subentity_keywords.contains(kw))
        {
            return {};
        }
        result = kw;

        if (kw == "OPERATOR")
        {
            skip_whitespace(pos, end);
            auto sym = parse_symbol(pos, end);
            if (sym.empty())
            {
                throw std::runtime_error("Expected operator symbol");
            }
            result += sym;
            skip_whitespace(pos, end);

            if (skip_keyword_if_is(pos, end, "RHS"))
            {
                result += "RHS";
            }
        }
        std::string remaining(pos, end);
        begin = pos;
        return result;
    }
} // namespace quxlang::parsers

#endif // PARSE_SUBENTITY_HPP
