//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef ITER_PARSE_KEYWORD_HPP
#define ITER_PARSE_KEYWORD_HPP

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
}

#endif //ITER_PARSE_KEYWORD_HPP
