// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_CLASS_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_CLASS_HEADER_GUARD

#include <optional>
#include <quxlang/ast2/ast2_type_map.hpp>

#include <quxlang/parsers/parse_class_body.hpp>

namespace quxlang::parsers
{
    inline std::optional< ast2_class_declaration > try_parse_class(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;
        std::optional< ast2_class_declaration > out;

        if (!skip_keyword_if_is(pos, end, "CLASS"))
        {
            return out;
        }

        skip_whitespace_and_comments(pos, end);

        out = parse_class_body(ctx);
        out->location = ctx.get_location_optional(begin, pos);
        return out;
    }

} // namespace quxlang::parsers

#endif // TRY_PARSE_CLASS_HEADER_GUARD
