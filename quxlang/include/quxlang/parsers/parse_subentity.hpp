//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef PARSE_SUBENTITY_HPP
#define PARSE_SUBENTITY_HPP
#include <string>
#include <set>
#include <quxlang/parsers/iter_parse_identifier.hpp>
#include <quxlang/parsers/iter_parse_keyword.hpp>

namespace quxlang::parsers
{
    template < typename It >
   inline std::string parse_subentity(It& begin, It end)
    {
        static std::set< std::string > subentity_keywords = {"CONSTRUCTOR", "DESTRUCTOR", "OPERATOR"};

        auto pos = iter_parse_identifier(begin, end);
        if (pos == begin)
        {
            pos = iter_parse_keyword(begin, end);
            auto kw = std::string(begin, pos);

            if (pos == begin || !subentity_keywords.contains(kw))
            {
                return {};
            }
        }
        auto result = std::string(begin, pos);
        begin = pos;
        return result;
    }

}

#endif //PARSE_SUBENTITY_HPP
