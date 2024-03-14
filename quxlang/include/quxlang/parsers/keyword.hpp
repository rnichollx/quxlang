//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef ITER_PARSE_KEYWORD_HPP
#define ITER_PARSE_KEYWORD_HPP

namespace quxlang::parsers
{
    template <typename It>
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

    template <typename It>
    inline bool skip_keyword_if_is(It& ipos, It end, std::string_view keyword)
    {
        auto pos = iter_parse_keyword(ipos, end);
        if (pos == ipos)
        {
            return false;
        }
        auto pos2 = ipos;
        std::string kw(pos2, pos);
        if (kw == std::string(keyword))
        {
            ipos = pos;
            return true;
        }
        return false;
    }

    template <typename It>
    std::string parse_keyword(It& pos, It end)
    {
        auto it = iter_parse_keyword(pos, end);
        std::string out(pos, it);
        pos = it;
        return out;
    }

    template <typename It>
    std::string next_keyword(It& pos, It end)
    {
        auto it = iter_parse_keyword(pos, end);
        return std::string(pos, it);
    }
}

#endif //ITER_PARSE_KEYWORD_HPP