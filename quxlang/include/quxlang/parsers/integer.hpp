// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_INTEGER_HEADER_GUARD
#define QUXLANG_PARSERS_INTEGER_HEADER_GUARD

#include <cstddef>  // for std::size_t
#include <iterator> // for It
#include <optional> // for std::optional
#include <string>   // for std::string

#include <quxlang/parsers/ctype.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::size_t parse_uinteger(It& ipos, It end)
    {
        std::string istr = "";
        while (ipos != end && is_digit(*ipos))
        {
            istr += *ipos;
            ++ipos;
        }

        // TODO: This might cut off the number if it is too large.
        return std::stoull(istr);
    }

    template < typename It >
    ssize_t parse_integer(It& ipos, It end)
    {
        std::string istr = "";
        bool negative = false;
        if (ipos != end && *ipos == '-')
        {
            negative = true;
            ++ipos;
        }
        while (ipos != end && is_digit(*ipos))
        {
            istr += *ipos;
            ++ipos;
        }

        // TODO: This might cut off the number if it is too large.
        auto num = std::stoull(istr);
        if (negative)
            return -num;
        else
            return num;
    }

    template < typename It >
    std::optional< ssize_t > try_parse_integer(It& ipos, It end)
    {
        std::string istr = "";
        while (ipos != end && is_digit(*ipos))
        {
            istr += *ipos;
            ++ipos;
        }

        if (istr.empty())
            return std::nullopt;

        return std::stoll(istr);
    }
} // namespace quxlang::parsers

#endif // RPNX_QUXLANG_INTEGER_HEADER