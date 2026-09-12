// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_ITER_PARSE_NUMBER_HEADER_GUARD
#define QUXLANG_PARSERS_ITER_PARSE_NUMBER_HEADER_GUARD

#include <quxlang/parsers/ctype.hpp>

namespace quxlang::parsers
{
    /** Scans a numeric literal, optionally accepting a leading minus in expression contexts. */
    template < typename It >
    constexpr auto iter_parse_number(It begin, It end, bool allow_negation = false) -> It
    {
        auto pos = begin;
        if (allow_negation && pos != end && *pos == '-')
        {
            ++pos;
        }
        if (pos != end && is_digit(*pos))
        {
            pos++;
        }
        else
        {
            return begin;
        }

        bool havedot = false;

        while (pos != end && (is_digit(*pos) || (*pos == '.' && !havedot)))
        {
            if (*pos == '.')
            {
                havedot = true;
            }
            pos++;
        }

        return pos;
    }
} // namespace quxlang::parsers

#endif // ITER_PARSE_NUMBER_HPP
