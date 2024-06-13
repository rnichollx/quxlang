//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef QUXLANG_TRY_PARSE_CLASS_HEADER_GUARD
#define QUXLANG_TRY_PARSE_CLASS_HEADER_GUARD

#include <optional>
#include <quxlang/ast2/ast2_type_map.hpp>

#include <quxlang/parsers/parse_class_body.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::optional< ast2_class_declaration > try_parse_class(It& pos, It end)
    {
        std::optional< ast2_class_declaration > out;

        if (!skip_keyword_if_is(pos, end, "CLASS"))
        {
            return out;
        }

        skip_whitespace_and_comments(pos, end);

        std::string remaining = std::string(pos, end);

        out = parse_class_body(pos, end);
        return out;
    }

    inline std::optional< ast2_class_declaration > try_parse_class(std::string str)
    {
        auto it = str.begin();
        return try_parse_class(it, str.end());
    }

} // namespace quxlang::parsers

#endif // TRY_PARSE_CLASS_HEADER_GUARD
