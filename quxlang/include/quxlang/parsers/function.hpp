//
// Created by Ryan Nicholl on 6/26/24.
//

#ifndef RPNX_QUXLANG_FUNCTION_HEADER
#define RPNX_QUXLANG_FUNCTION_HEADER

#include <quxlang/parsers/parse_identifier.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <set>
#include <string>

namespace quxlang::parsers
{
    template < typename It >
    std::string parse_argument_name(It& pos, It end)
    {
        static std::set< std::string > argument_keywords = {"THIS", "RETURN", "OTHER"};

        auto identifier = parse_identifier(pos, end);
        if (identifier.empty())
        {
            auto keyword_end = iter_parse_keyword(pos, end);
            std::string keyword(pos, keyword_end);
            if (argument_keywords.contains(keyword))
            {
                pos = keyword_end;
                return keyword;
            }
            else
            {
                throw std::runtime_error("Expected identifier or keyword");
            }
        }

        return identifier;
    }
} // namespace quxlang::parsers

#endif // RPNX_QUXLANG_FUNCTION_HEADER
