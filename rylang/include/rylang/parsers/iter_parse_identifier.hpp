//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef ITER_PARSE_IDENTIFIER_HPP
#define ITER_PARSE_IDENTIFIER_HPP

namespace rylang::parsers
{
    template < typename It >
    auto iter_parse_identifier(It begin, It end) -> It
    {
        auto pos = begin;
        if (pos == end)
        {
            return pos;
        }
        bool was_underscore = false;
        bool started = false;
        while (pos != end)
        {
            char c = *pos;
            if ((c >= 'a' && c <= 'z') || (started && ((c >= '0' && c <= '9') || c == '_')))

            {
                was_underscore = c == '_';
                ++pos;
                started = true;
            }
            else
                break;
        }
        if (was_underscore)
        {
            return begin;
        }
        return pos;
    }
} // namespace rylang::parsers

#endif // ITER_PARSE_IDENTIFIER_HPP
