// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_SKIP_WHITESPACE_HEADER_GUARD
#define QUXLANG_PARSERS_SKIP_WHITESPACE_HEADER_GUARD
#include "context.hpp"

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

    inline constexpr bool skip_whitespace2(parsing_context & ctx)
    {
        bool skipped_anything = false;
        while (ctx.iter_pos != ctx.iter_end && (*ctx.iter_pos == ' ' || *ctx.iter_pos == '\t' || *ctx.iter_pos == '\n' || *ctx.iter_pos == '\r'))
        {
            ++ctx.iter_pos;
            skipped_anything = true;
        }

        return skipped_anything;
    }
} // namespace quxlang::parsers

#endif // SKIP_WHITESPACE_HPP
