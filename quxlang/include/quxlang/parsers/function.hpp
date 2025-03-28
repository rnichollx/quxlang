// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_FUNCTION_HEADER_GUARD
#define QUXLANG_PARSERS_FUNCTION_HEADER_GUARD

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
                throw std::logic_error("Expected identifier or keyword");
            }
        }

        return identifier;
    }
} // namespace quxlang::parsers

#endif // RPNX_QUXLANG_FUNCTION_HEADER
