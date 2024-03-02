//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef PARSE_WHITESPACE_AND_COMMENTS_HPP
#define PARSE_WHITESPACE_AND_COMMENTS_HPP

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
