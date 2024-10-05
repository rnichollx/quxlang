// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_COMMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_COMMENT_HEADER_GUARD

#include <quxlang/parsers/iter_parse_line_comment.hpp>

namespace quxlang::parsers
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
} // namespace quxlang

#endif // SKIP_COMMENT_HPP
