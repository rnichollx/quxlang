//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef QUXLANG_PARSERS_PARSE_WHITESPACE_AND_COMMENTS_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_WHITESPACE_AND_COMMENTS_HEADER_GUARD

#include <quxlang/parsers/parse_comment.hpp>
#include <quxlang/parsers/skip_whitespace.hpp>
#include <quxlang/parsers/parse_keyword.hpp>

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
} // namespace quxlang

#endif // PARSE_WHITESPACE_AND_COMMENTS_HPP
