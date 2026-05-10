// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_FUNCTION_HEADER_GUARD
#define QUXLANG_PARSERS_FUNCTION_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

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
        static constexpr std::array< std::string_view, 16 > argument_keywords = {
            "T",
            "THIS",
            "RETURN",
            "OTHER",
            "VALUE",
            "SUCCESS",
            "FAILURE",
            "EXPLICIT",
            "REINTERPRET",
            "CHECKED",
            "ASSUME",
            "PARTIAL",
            "APPROXIMATE",
            "OUTPUT_ITERATOR",
            "INPUT_ITERATOR",
            "DESERIALIZE_INPUT_ITERATOR",
        };

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
            throw syntax_compilation_error("Expected identifier or keyword");
        }

        return identifier;
    }
} // namespace quxlang::parsers

#endif // RPNX_QUXLANG_FUNCTION_HEADER
