//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef RYLANG_SKIP_WHITESPACE_HPP
#define RYLANG_SKIP_WHITESPACE_HPP

namespace rylang::parsers
{
    template < typename It >
    constexpr bool skip_whitespace(It& pos, It end)
    {
        bool skipped_anything = false;
        while (pos != end && (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r'))
        {
            ++pos;
            skipped_anything = true;
        }

        return skipped_anything;
    }
} // namespace rylang::parsers

#endif // SKIP_WHITESPACE_HPP
