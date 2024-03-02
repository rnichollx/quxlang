//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef NEXT_KEYWORD_HPP
#define NEXT_KEYWORD_HPP


#include <string>
#include <quxlang/parsers/parse_keyword.hpp>


namespace quxlang
{
    template <typename It>
    std::string next_keyword(It pos, It end)
    {
        return parse_keyword(pos, end);
    }
}

#endif //NEXT_KEYWORD_HPP
