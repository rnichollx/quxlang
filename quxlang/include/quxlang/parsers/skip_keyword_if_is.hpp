//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef QUXLANG_SKIP_KEYWORD_IF_IS_HPP
#define QUXLANG_SKIP_KEYWORD_IF_IS_HPP

#include <quxlang/parsers/iter_parse_keyword.hpp>
#include <string_view>

namespace quxlang::parsers
{
    template < typename It >
    inline bool skip_keyword_if_is(It& ipos, It end, std::string_view keyword)
    {
        auto pos = iter_parse_keyword(ipos, end);
        if (pos == ipos)
        {
            return false;
        }
        auto pos2 = ipos;
        std::string kw(pos2, pos);
        if (kw == std::string(keyword))
        {
            ipos = pos;
            return true;
        }
        return false;
    }
} // namespace quxlang

#endif // SKIP_KEYWORD_IF_IS_HPP
