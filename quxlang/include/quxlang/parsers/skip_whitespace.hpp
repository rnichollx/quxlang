// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_SKIP_WHITESPACE_HEADER_GUARD
#define QUXLANG_PARSERS_SKIP_WHITESPACE_HEADER_GUARD

namespace quxlang::parsers
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
} // namespace quxlang::parsers

#endif // SKIP_WHITESPACE_HPP
