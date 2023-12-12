//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef SKIP_COMMENT_HPP
#define SKIP_COMMENT_HPP

#include <rylang/parsers/iter_parse_line_comment.hpp>

namespace rylang::parsers
{
    template < typename It >
    inline bool skip_comment(It& begin, It end)
    {
        auto pos = iter_parse_line_comment(begin, end);
        if (pos == begin)
        {
            return false;
        }
        begin = pos;
        return true;
    }
} // namespace rylang

#endif // SKIP_COMMENT_HPP
