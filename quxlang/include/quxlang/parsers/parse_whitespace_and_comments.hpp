// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_WHITESPACE_AND_COMMENTS_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_WHITESPACE_AND_COMMENTS_HEADER_GUARD

#include "context.hpp"

#include <quxlang/parsers/parse_comment.hpp>
#include <quxlang/parsers/parse_keyword.hpp>
#include <quxlang/parsers/skip_whitespace.hpp>

namespace quxlang::parsers
{
    template < typename It >
    bool skip_whitespace_and_comments(It& pos, It end)
    {
        if (skip_whitespace(pos, end) || skip_comment(pos, end))
        {
            while (skip_whitespace(pos, end) || skip_comment(pos, end)) {}
            return true;
        }
        else
        {
            return false;
        }
    }

    inline bool skip_whitespace_and_comments2(parsing_context & ctx)
    {
        if (skip_whitespace2(ctx) || skip_comment2(ctx))
        {
            while (skip_whitespace2(ctx) || skip_comment2(ctx)) {}
            return true;
        }
        else
        {
            return false;
        }
    }
} // namespace quxlang

#endif // PARSE_WHITESPACE_AND_COMMENTS_HPP
