//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef RYLANG_TRY_PARSE_CLASS_HEADER_GUARD
#define RYLANG_TRY_PARSE_CLASS_HEADER_GUARD

#include <optional>
#include <rylang/ast2/ast2_class.hpp>

#include <rylang/parsers/parse_class_body.hpp>

namespace rylang::parsers
{
    template < typename It >
    std::optional< ast2_class > try_parse_class(It& pos, It end)
    {
        std::optional< ast2_class > out;

        if (!skip_keyword_if_is(pos, end, "CLASS"))
        {
            return out;
        }

        out = parse_class_body(pos, end);
        return out;
    }

    std::optional< ast2_class > try_parse_class(std::string str)
    {
        auto it = str.begin();
        return try_parse_class(it, str.end());
    }

} // namespace rylang::parsers

#endif // TRY_PARSE_CLASS_HEADER_GUARD
