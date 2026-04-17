// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_FUNCTION_HEADER_GUARD
#define QUXLANG_PARSERS_FUNCTION_HEADER_GUARD

#include <quxlang/parsers/parse_identifier.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <algorithm>
#include <array>
#include <iterator>
#include <string>
#include <string_view>

namespace quxlang::parsers
{
    template < typename It >
    std::string parse_argument_name(It& pos, It end)
    {
        static constexpr std::array< std::string_view, 9 > argument_keywords = {"THIS", "RETURN", "OTHER", "EXPLICIT", "CHECKED", "ASSUME", "PARTIAL", "OUTPUT_ITERATOR", "INPUT_ITER"};

        auto identifier = parse_identifier(pos, end);
        if (identifier.empty())
        {
            auto keyword_end = iter_parse_keyword(pos, end);
            for (auto keyword : argument_keywords)
            {
                if (static_cast< std::size_t >(std::distance(pos, keyword_end)) == keyword.size() && std::equal(pos, keyword_end, keyword.begin(), keyword.end()))
                {
                    pos = keyword_end;
                    return std::string(keyword);
                }
            }
            throw std::logic_error("Expected identifier or keyword");
        }

        return identifier;
    }
} // namespace quxlang::parsers

#endif // RPNX_QUXLANG_FUNCTION_HEADER
