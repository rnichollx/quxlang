// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_KEYWORD_HEADER_GUARD
#define QUXLANG_PARSERS_KEYWORD_HEADER_GUARD

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>

namespace quxlang::parsers
{

    template < typename It >
    inline auto iter_parse_keyword(It begin, It end) -> It
    {
        bool started = false;
        auto pos = begin;
        while (pos != end && ((*pos == '_') || (*pos >= 'A' && *pos <= 'Z') || (started && (*pos >= '0' && *pos <= '9'))))
        {
            ++pos;
            started = true;
        }
        return pos;
    }

    template < typename It >
    inline bool skip_keyword_if_is(It& ipos, It end, std::string_view keyword)
    {
        auto pos = iter_parse_keyword(ipos, end);
        if (pos == ipos)
        {
            return false;
        }
        if (static_cast< std::size_t >(std::distance(ipos, pos)) == keyword.size() && std::equal(ipos, pos, keyword.begin(), keyword.end()))
        {
            ipos = pos;
            return true;
        }
        return false;
    }

    template < typename It >
    inline std::optional< std::string_view > skip_keyword_if_one_of(It& ipos, It end, std::initializer_list< std::string_view > keywords)
    {
        for (auto kw : keywords)
        {
            if (skip_keyword_if_is(ipos, end, kw))
            {
                return kw;
            }
        }
        return std::nullopt;
    }

    template < typename It >
    inline std::string skip_keyword(It& ipos, It end)
    {
        auto pos = iter_parse_keyword(ipos, end);
        if (pos == ipos)
        {
            return "";
        }
        auto pos2 = ipos;
        std::string kw(pos2, pos);
        ipos = pos;
        return kw;
    }

    template < typename It >
    std::string parse_keyword(It& pos, It end)
    {
        auto it = iter_parse_keyword(pos, end);
        std::string out(pos, it);
        pos = it;
        return out;
    }

    template < typename It >
    std::string next_keyword(It& pos, It end)
    {
        auto it = iter_parse_keyword(pos, end);
        return std::string(pos, it);
    }
} // namespace quxlang::parsers

#endif // ITER_PARSE_KEYWORD_HPP
